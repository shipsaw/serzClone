package main

import (
	"bytes"
	"container/list"
	"encoding/binary"
	"encoding/xml"
	"fmt"
	"os"
	"strconv"
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
	symbolMap := make(map[uint16]string)
	fmt.Println(symbolMap)

	depthList := list.New()

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
	var depthElem *list.Element
	walk([]Node{n}, func(n Node) bool {
		children := len(n.Nodes)
		if children > 0 { // If an FF 50 newline
			depthElem = depthList.PushBack(listElem{depthCounter, children})
			printList(depthList)
			depthCounter++
			writeFF50(n, buf)
		} else { // If an FF 56 or FF 41
			writeFF56(n, buf)

			// decrement children count
			newElement := listElem{
				depth:    depthElem.Value.(listElem).depth,
				children: depthElem.Value.(listElem).children - 1,
			}
			depthList.Remove(depthElem)
			depthElem = depthList.PushBack(newElement)
			printList(depthList)
		}

		// If we have processed all the children of a parent, unwind and write FF 70, which indicates closing element
		if depthElem.Value.(listElem).children == 0 {
			err := binary.Write(buf, binary.BigEndian, []byte("\xFF\x70"))
			if err != nil {
				panic(err)
			}
			err = binary.Write(buf, binary.LittleEndian, int16(depthElem.Value.(listElem).depth))
			if err != nil {
				panic(err)
			}
			depthList.Remove(depthElem)
			for elem := depthList.Back(); elem != nil && elem.Value.(listElem).children == 1; elem = elem.Prev() {
				err = binary.Write(buf, binary.BigEndian, []byte("\xFF\xFF"))
				if err != nil {
					panic(err)
				}
				err = binary.Write(buf, binary.LittleEndian, int16(elem.Value.(listElem).depth))
				if err != nil {
					panic(err)
				}
			}
		}

		return true
	})
	printBin(buf)
}

func printList(depthList *list.List) {
	for elem := depthList.Front(); elem != nil; elem = elem.Next() {
		fmt.Printf("%v", elem.Value)
	}
	fmt.Printf("\n")
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

func writeFF50(n Node, buf *bytes.Buffer) {
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

func writeFF56(n Node, buf *bytes.Buffer) {
	// Write the Newline sequence
	err := binary.Write(buf, binary.BigEndian, []byte("\xFF\x56\xFF\xFF"))
	if err != nil {
		panic(err)
	}
	// Write the length of the xml element name
	nameLen := int32(len(n.XMLName.Local))
	err = binary.Write(buf, binary.LittleEndian, nameLen)
	if err != nil {
		panic(err)
	}
	// Write the element name
	err = binary.Write(buf, binary.BigEndian, []byte(n.XMLName.Local))
	if err != nil {
		panic(err)
	}

	fmt.Println("WRITING FF FOR ATTRIBUTE" + n.XMLName.Local)
	// Write the FF FF bytes, which signals this is a new attribute TODO Update this!
	err = binary.Write(buf, binary.BigEndian, []byte("\xFF\xFF"))
	if err != nil {
		panic(err)
	}

	// Write the length of the d:type attribute
	attrNameLen := int32(0)
	for _, attr := range n.Attrs {
		if attr.Name.Local == "type" {
			tempLen := len(attr.Value)
			attrNameLen = int32(tempLen)
			err = binary.Write(buf, binary.LittleEndian, attrNameLen)
			if err != nil {
				panic(err)
			}
			// Write the d:type attribute value
			err = binary.Write(buf, binary.LittleEndian, []byte(attr.Value))
			if err != nil {
				panic(err)
			}

			// Write the content value
			convertContent(attr.Value, string(n.Content), buf)
		}
	}
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
func convertContent(dType string, num string, buf *bytes.Buffer) {
	switch dType {
	case "bool":
		if num == "0" {
			err := binary.Write(buf, binary.LittleEndian, []byte("\x00"))
			if err != nil {
				panic(err)
			}
		} else {
			err := binary.Write(buf, binary.LittleEndian, []byte("\x01"))
			if err != nil {
				panic(err)
			}
		}
	case "sFloat32":
		flt64, err := strconv.ParseFloat(num, 32)
		if err != nil {
			panic(err)
		}
		err = binary.Write(buf, binary.LittleEndian, float32(flt64))
		if err != nil {
			panic(err)
		}
	case "cDeltaString":
		// Write the FF designation TODO update this
		err := binary.Write(buf, binary.BigEndian, []byte("\xFF\xFF"))
		if err != nil {
			panic(err)
		}

		// Write the length of the string first
		strLen := len(num)
		err = binary.Write(buf, binary.LittleEndian, int32(strLen))
		if err != nil {
			panic(err)
		}
		// Now write the string
		err = binary.Write(buf, binary.BigEndian, []byte(num))
		if err != nil {
			panic(err)
		}
	}
}
