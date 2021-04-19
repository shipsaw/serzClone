package main

import (
	"bytes"
	"encoding/binary"
	"encoding/xml"
	"fmt"
	"os"
	"strconv"
	"strings"
)

const prelude string = "SERZ\x00\x00\x01\x00"

// When decoded, each line of XML is unpacked into a struct; in this case it's a generic struct that can work for every XML entry type
type Node struct {
	XMLName xml.Name
	Content []byte     `xml:",innerxml"`
	Nodes   []Node     `xml:",any"`
	Attrs   []xml.Attr `xml:",any,attr"`
}

type listElem struct {
	depth    uint16 // Line number to return to
	children int    // Data size matches bin representation
}

func main() {
	// Create a symbols map so that so that no names are duplicated in the bin
	symbolMap := make(map[string]uint16)

	depthList := []listElem{}

	// Create output buffer, write the prelude, then open and decode the xml document
	buf := new(bytes.Buffer)
	err := binary.Write(buf, binary.BigEndian, []byte(prelude))
	if err != nil {
		fmt.Println("binary.Write failed:", err)
	}

	fileBytes, err := os.ReadFile("./test.xml")
	if err != nil {
		panic(err)
	}
	filebuf := bytes.NewBuffer(fileBytes)
	decoder := xml.NewDecoder(filebuf)

	var n Node
	err = decoder.Decode(&n)
	if err != nil {
		panic(err)
	}

	var depthCounter uint16 = 0
	walk([]Node{n}, func(n Node) bool {
		children := len(n.Nodes)
		if children > 0 { // If an FF 50 newline
			depthList = append(depthList, listElem{depthCounter, children})
			//fmt.Println(depthList)
			depthCounter++
			writeFF50(n, buf, symbolMap)
		} else { // If an FF 56 or FF 41
			writeFF56_41(n, buf, symbolMap)

			// decrement children count
			depthList[len(depthList)-1].children--
			//fmt.Println(depthList)
		}

		// Check to see if we are at the end of a set of children.  If we are, unwind back to the next element with children,
		// Printing FF 70's along the way
		lastElem := depthList[len(depthList)-1]
		if lastElem.children == 0 {
			err := binary.Write(buf, binary.BigEndian, []byte("\xFF\x70"))
			if err != nil {
				panic(err)
			}
			err = binary.Write(buf, binary.LittleEndian, int16(lastElem.depth))
			if err != nil {
				panic(err)
			}
			depthList = depthList[:len(depthList)-1]

			listLen := len(depthList) - 1
			for i := listLen; i >= 0 && depthList[i].children == 1; i-- {
				err = binary.Write(buf, binary.BigEndian, []byte("\xFF\x70"))
				if err != nil {
					panic(err)
				}
				err = binary.Write(buf, binary.LittleEndian, int16(depthList[i].depth))
				if err != nil {
					panic(err)
				}
				depthList = depthList[:len(depthList)-1]
			}
		}

		return true
	})
	printBin(buf)
	fmt.Println(symbolMap)
}

// Function walk works it's way through the xml nodes, temporarily storing each element in a struct, evaluating it, then moving on
// to the next node.
func walk(nodes []Node, f func(Node) bool) {
	for _, n := range nodes {
		if f(n) {
			walk(n.Nodes, f)
		}
	}
}

func writeFF50(n Node, buf *bytes.Buffer, symbolMap map[string]uint16) {
	// Put the element name into the symbolMap
	if _, ok := symbolMap[n.XMLName.Local]; !ok {
		symbolMap[n.XMLName.Local] = uint16(len(symbolMap))
	}
	// Write the Newline sequence
	err := binary.Write(buf, binary.BigEndian, []byte("\xFF\x50\xFF\xFF"))
	if err != nil {
		panic(err)
	}
	// Write the length of the xml element name
	nameLen := int32(len(n.XMLName.Local))
	err = binary.Write(buf, binary.LittleEndian, nameLen)
	if err != nil {
		panic(err)
	}
	//Write the element name
	err = binary.Write(buf, binary.BigEndian, []byte(n.XMLName.Local))
	if err != nil {
		panic(err)
	}
	//Write the d:id value
	var idVal int32 = 0
	for _, attr := range n.Attrs {
		if attr.Name.Local == "id" {
			tempVal, err := strconv.Atoi(attr.Value)
			if err != nil {
				panic(err)
			}
			idVal = int32(tempVal)
		}
	}
	err = binary.Write(buf, binary.LittleEndian, idVal)
	if err != nil {
		panic(err)
	}

	// Write the number of children the current element has
	childCount := int32(len(n.Nodes))
	err = binary.Write(buf, binary.LittleEndian, childCount)
	if err != nil {
		panic(err)
	}
}

func writeFF56_41(n Node, buf *bytes.Buffer, symbolMap map[string]uint16) {
	// variables to keep track of our symbols, and whether they need to be written out or can be written shorthand with
	// their reference value
	nameMatch := false
	attrMatch := false
	var numElements uint8 = 1
	// Look for type attribute now because we need to see if that symbol can be swapped out for it's mapped value
	typeAttr := xml.Attr{}
	for _, attr := range n.Attrs {
		if attr.Name.Local == "type" || attr.Name.Local == "elementType" {
			typeAttr = attr
			if _, nameMatch = symbolMap[n.XMLName.Local]; !nameMatch { // Check for XML name match
				symbolMap[n.XMLName.Local] = uint16(len(symbolMap))
			}
			if _, attrMatch = symbolMap[attr.Value]; !attrMatch { // Check for d:type attribute value match
				symbolMap[attr.Value] = uint16(len(symbolMap))
			}
		}
		if attr.Name.Local == "numElements" {
			temp, err := strconv.Atoi(attr.Value)
			numElements = uint8(temp)
			if err != nil {
				panic(err)
			}
		}
	}
	// Write the Newline sequence
	if numElements > 1 { // If more than one element, thus FF 41
		err := binary.Write(buf, binary.BigEndian, []byte("\xFF\x41\xFF\xFF"))
		if err != nil {
			panic(err)
		}
	} else {
		err := binary.Write(buf, binary.BigEndian, []byte("\xFF\x56\xFF\xFF"))
		if err != nil {
			panic(err)
		}
	}

	if nameMatch {
		err := binary.Write(buf, binary.LittleEndian, symbolMap[n.XMLName.Local])
		if err != nil {
			panic(err)
		}
	} else {
		// Write the length of the xml element name
		nameLen := int32(len(n.XMLName.Local))
		err := binary.Write(buf, binary.LittleEndian, nameLen)
		if err != nil {
			panic(err)
		}
		// Write the element name
		err = binary.Write(buf, binary.BigEndian, []byte(n.XMLName.Local))
		if err != nil {
			panic(err)
		}
	}

	// Write the FF FF bytes or name match reference number
	if attrMatch {
		err := binary.Write(buf, binary.LittleEndian, symbolMap[typeAttr.Value])
		if err != nil {
			panic(err)
		}
	} else {
		err := binary.Write(buf, binary.BigEndian, []byte("\xFF\xFF"))
		if err != nil {
			panic(err)
		}

		// Write the length of the d:type attribute
		tempLen := len(typeAttr.Value)
		attrNameLen := int32(tempLen)
		err = binary.Write(buf, binary.LittleEndian, attrNameLen)
		if err != nil {
			panic(err)
		}
		// Write the d:type attribute value
		err = binary.Write(buf, binary.LittleEndian, []byte(typeAttr.Value))
		if err != nil {
			panic(err)
		}

	}

	// if FF 41, write numElements value
	if numElements > 1 {
		err := binary.Write(buf, binary.LittleEndian, numElements)
		if err != nil {
			panic(err)
		}
	}
	// Write the content value
	convertContent(typeAttr.Value, string(n.Content), buf, symbolMap)

}

// Print out the binary file
func printBin(buf *bytes.Buffer) {
	fmt.Printf("\n")
	fmt.Printf("1   ")
	for i, byt := range buf.Bytes() {
		fmt.Printf("%02X ", byt)
		if linenum := (i + 1) % 16; linenum == 0 && i+1 < len(buf.Bytes()) {
			fmt.Printf("\n%-4d", (i+1)/16+1)
		}
	}
	fmt.Printf("\n\n")
}

// Content has been encoded in various types, but is always a string in the xml file. This function
// converts the string to the appropriate number type, than writes it to the buffer

func convertContent(dType string, num string, buf *bytes.Buffer, symbolMap map[string]uint16) {
	// Args will work with the case of FF 41, where there are multiple content values.  It takes the values and splits them,
	// then loops trough the conversion switch.  If the content of the element is a string, splitting it by spaces makes
	// no sense, so splitting is skipped
	var args = *new([]string)
	if dType != "cDeltaString" {
		args = strings.Fields(num)
	} else {
		args = append(args, num)
	}

	for _, arg := range args {
		switch dType {
		case "bool":
			ret := make([]byte, 1)
			if arg == "0" {
				ret = []byte("\x00")
			} else {
				ret = []byte("\x01")
			}
			err := binary.Write(buf, binary.LittleEndian, ret)
			if err != nil {
				panic(err)
			}
		case "sFloat32":
			flt64, err := strconv.ParseFloat(arg, 32)
			if err != nil {
				panic(err)
			}
			err = binary.Write(buf, binary.LittleEndian, float32(flt64))
			if err != nil {
				panic(err)
			}
		case "sInt32":
			newInt, err := strconv.Atoi(arg)
			if err != nil {
				panic(err)
			}
			err = binary.Write(buf, binary.LittleEndian, int32(newInt))
			if err != nil {
				panic(err)
			}
		case "sUInt8":
			newInt, err := strconv.Atoi(arg)
			if err != nil {
				panic(err)
			}
			err = binary.Write(buf, binary.LittleEndian, uint8(newInt))
			if err != nil {
				panic(err)
			}
		case "cDeltaString":
			if _, ok := symbolMap[arg]; ok { // If this string is already saved in our symbol table...
				err := binary.Write(buf, binary.LittleEndian, symbolMap[arg])
				if err != nil {
					panic(err)
				}
			} else {
				// Write the FF designation
				err := binary.Write(buf, binary.BigEndian, []byte("\xFF\xFF"))
				if err != nil {
					panic(err)
				}

				// Write the length of the string first
				strLen := len(arg)
				err = binary.Write(buf, binary.LittleEndian, int32(strLen))
				if err != nil {
					panic(err)
				}
				// Now write the string
				err = binary.Write(buf, binary.BigEndian, []byte(num))
				if err != nil {
					panic(err)
				}
				symbolMap[arg] = uint16(len(symbolMap))
			}
		}
	}
}
