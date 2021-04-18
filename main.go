package main

import (
	"bytes"
	"encoding/binary"
	"encoding/xml"
	"fmt"
	"os"
	"strconv"
)

type Node struct {
	XMLName xml.Name
	Content []byte     `xml:",innerxml"`
	Nodes   []Node     `xml:",any"`
	Attrs   []xml.Attr `xml:",any,attr"`
}

func main() {
	buf := new(bytes.Buffer)
	prelude := "SERZ\x00\x00\x01\x00"
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

	walk([]Node{n}, func(n Node) bool {
		if len(n.Nodes) > 0 {
			writeFF50(n, buf)
		}
		return true
	})
	fmt.Printf("% x", buf.Bytes())
}

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

	childCount := int32(len(n.Nodes))
	err = binary.Write(buf, binary.LittleEndian, childCount)
	if err != nil {
		panic(err)
	}
}

/*
func (n *Node) UnmarshalXML(d *xml.Decoder, start xml.StartElement) error {
	n.Attrs = start.Attr
	type node Node

	return d.DecodeElement((*node)(n), &start)
}
*/
