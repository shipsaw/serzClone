package main

import (
	"encoding/binary"
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"regexp"
	"strconv"
	"strings"

	"github.com/pkg/profile"
)

var cpuprofile = flag.String("cpuprofile", "", "write cpu profile to `file`")
var memprofile = flag.String("memprofile", "", "write memory profile to `file`")

const (
	PREFIX string = "SERZ\x00\x00\x01\x00"
	FF50   string = "\xFF\x50\xFF\xFF"
	FF41   string = "\xFF\x41\xFF\xFF"
	FF56   string = "\xFF\x56\xFF\xFF"
	FF70   string = "\xFF\x70"
	FF     string = "\xFF\xFF"
)

type output struct {
	outBytes  []byte
	writePos  int
	symbolMap symMap
	tagRecord []tag
	tagIndex  int
}

type symMap struct {
	sMap  map[string]int
	index int
}

type tag struct {
	children int
	lineNum  int
	dWordLoc int
}

func outputCreate(bytes int, tagLen int) output {
	return output{
		outBytes: make([]byte, bytes),
		writePos: 0,
		symbolMap: symMap{
			sMap: make(map[string]int),
		},
		tagRecord: make([]tag, tagLen),
	}
}

func (o *output) Write(p []byte) (n int, err error) {
	for i := 0; i < len(p); i++ {
		o.outBytes[o.writePos+i] = p[i]
	}
	o.writePos += len(p)
	return len(p), nil
}

func (o *output) WriteMid(p int, pos int) error {
	temp := o.writePos
	o.writePos = pos
	err := binary.Write(o, binary.BigEndian, uint32(p))
	if err != nil {
		return err
	}
	o.writePos = temp
	return nil
}

func (o *output) SymExists(symbol string) (bool, int) {
	if tableVal, ok := o.symbolMap.sMap[symbol]; ok {
		return true, tableVal
	}
	o.symbolMap.sMap[symbol] = o.symbolMap.index
	o.symbolMap.index++
	return false, -1
}

func (o *output) writePrefix() error {
	err := binary.Write(o, binary.BigEndian, []byte(PREFIX))
	if err != nil {
		return err
	}
	return nil
}

func (o *output) Write50(matches []string, lineNum int) error {
	if lineNum > 0 { // If not first row
		o.tagRecord[o.tagIndex].children++
		o.tagIndex++
	}
	// matches: Name, d:id
	o.tagRecord[o.tagIndex].lineNum = lineNum
	// FF 50 FF FF
	err := binary.Write(o, binary.BigEndian, []byte(FF50))
	if err != nil {
		return err
	}

	// Check for symbol duplicate
	// Name len
	err = binary.Write(o, binary.LittleEndian, uint32(len(matches[0])))
	if err != nil {
		return err
	}
	// Name
	exists, num := o.SymExists(matches[0])
	if !exists {
		err = binary.Write(o, binary.BigEndian, []byte(matches[0]))
		if err != nil {
			return err
		}
	} else {
		err = binary.Write(o, binary.LittleEndian, uint16(num))
		if err != nil {
			return err
		}
	}
	// d:id
	id := 0
	if matches[1] != "" {
		id, err = strconv.Atoi(matches[1])
		if err != nil {
			return err
		}
	}
	err = binary.Write(o, binary.LittleEndian, uint32(id))
	if err != nil {
		return err
	}
	// Num children
	// First, save the write position of this children byte sequence
	o.tagRecord[o.tagIndex].dWordLoc = o.writePos
	err = binary.Write(o, binary.BigEndian, uint32(0))
	if err != nil {
		return err
	}

	return nil
}

func (o *output) Write56(matches []string) error {
	// matches: Name, Type, Content
	o.tagRecord[o.tagIndex].children++
	// FF 56 FF FF
	err := binary.Write(o, binary.BigEndian, []byte(FF56))
	if err != nil {
		return err
	}
	// Name len
	err = binary.Write(o, binary.LittleEndian, uint32(len(matches[0])))
	if err != nil {
		return err
	}
	// Name
	exists, num := o.SymExists(matches[0])
	if !exists {
		err = binary.Write(o, binary.BigEndian, []byte(matches[0]))
		if err != nil {
			return err
		}
	} else {
		err = binary.Write(o, binary.LittleEndian, uint16(num))
		if err != nil {
			return err
		}
	}
	exists, num = o.SymExists(matches[1])
	if !exists {
		// Type len prefix
		err = binary.Write(o, binary.BigEndian, []byte(FF))
		if err != nil {
			return err
		}
		// Type len
		err = binary.Write(o, binary.LittleEndian, uint32(len(matches[1])))
		if err != nil {
			return err
		}
		// Type
		err = binary.Write(o, binary.BigEndian, []byte(matches[1]))
		if err != nil {
			return err
		}
	} else {
		err = binary.Write(o, binary.LittleEndian, uint16(num))
		if err != nil {
			return err
		}
	}

	// Content
	if err := o.convertContent(matches[1], matches[2]); err != nil {
		return err
	}

	return nil
}

func (o *output) Write41(matches []string) error {
	// matches: Name, numElements, type, content
	o.tagRecord[o.tagIndex].children++
	// FF 41 FF FF
	err := binary.Write(o, binary.BigEndian, []byte(FF41))
	if err != nil {
		return err
	}
	// Name len
	err = binary.Write(o, binary.LittleEndian, uint32(len(matches[0])))
	if err != nil {
		return err
	}
	// Name
	exists, num := o.SymExists(matches[0])
	if !exists {
		err = binary.Write(o, binary.BigEndian, []byte(matches[0]))
		if err != nil {
			return err
		}
	} else {
		err = binary.Write(o, binary.LittleEndian, uint16(num))
		if err != nil {
			return err
		}
	}

	exists, num = o.SymExists(matches[2])
	if !exists {
		// Type len prefix
		err = binary.Write(o, binary.BigEndian, []byte(FF))
		if err != nil {
			return err
		}
		// Type len
		err = binary.Write(o, binary.LittleEndian, uint32(len(matches[2])))
		if err != nil {
			return err
		}

		// Type
		err = binary.Write(o, binary.BigEndian, []byte(matches[2]))
		if err != nil {
			return err
		}
	} else {
		err = binary.Write(o, binary.LittleEndian, uint16(num))
		if err != nil {
			return err
		}
	}
	// Content count
	cCount, err := strconv.Atoi(matches[1])
	if err != nil {
		return err
	}

	err = binary.Write(o, binary.BigEndian, uint8(cCount))
	if err != nil {
		return err
	}
	// Content
	if err := o.convertContent(matches[2], matches[3]); err != nil {
		return err
	}
	return nil
}

func (o *output) Write70() error {
	err := binary.Write(o, binary.BigEndian, []byte(FF70))
	if err != nil {
		return err
	}
	err = binary.Write(o, binary.LittleEndian, uint16(o.tagRecord[o.tagIndex].lineNum))
	if err != nil {
		return err
	}
	err = o.WriteMid(o.tagRecord[o.tagIndex].children, o.tagRecord[o.tagIndex].dWordLoc)
	if err != nil {
		return err
	}
	o.tagIndex--
	return nil
}

func (o *output) printBin() {
	fmt.Printf("\n")
	fmt.Printf("0   ")
	for i, byt := range o.outBytes[:o.writePos] {
		fmt.Printf("%02X ", byt)
		if linenum := (i + 1) % 16; linenum == 0 && i+1 < len(o.outBytes[:o.writePos]) {
			fmt.Printf("\n%-4d", (i+1)/16)
		}
	}
	fmt.Printf("\n\n")
}

var ff50patt string = `<(\w+)(?:.*?d:id="(\d{7,10})")?`
var ff56patt string = `<(\w+) (?:d:type="(\w+?)".*?>(.+?))<`
var ff41patt string = `<(\w+) d:numElements="(\d+)" d:elementType="(\w+)" .+?>(.+?)<`
var ff70patt string = `</(\w+?)>`

func main() {
	defer profile.Start().Stop()

	ff50regxp := regexp.MustCompile(ff50patt)
	ff56regxp := regexp.MustCompile(ff56patt)
	ff41regxp := regexp.MustCompile(ff41patt)
	ff70regxp := regexp.MustCompile(ff70patt)

	/*
		//symbolMap := new(symbolMap)
			if len(os.Args) <= 1 {
				fmt.Printf("USAGE : %s <target_filename> \n", os.Args[0])
				os.Exit(0)
			}

			fileName := os.Args[1]
	*/

	fileBytes, err := ioutil.ReadFile("test.xml")

	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}

	sliceData := strings.Split(string(fileBytes), "\n")
	output := outputCreate(5500000, len(sliceData))

	output.writePrefix()
	for i, slice := range sliceData {
		matches := ff41regxp.FindStringSubmatch(slice)
		if len(matches) > 0 {
			err = output.Write41(matches[1:])
			if err != nil {
				panic(err)
			}
			continue
		}
		matches = ff56regxp.FindStringSubmatch(slice)
		if len(matches) > 0 {
			err = output.Write56(matches[1:])
			if err != nil {
				panic(err)
			}
			continue
		}
		matches = ff50regxp.FindStringSubmatch(slice)
		if len(matches) > 0 {
			err = output.Write50(matches[1:], i)
			if err != nil {
				panic(err)
			}
			continue
		}
		matches = ff70regxp.FindStringSubmatch(slice)
		if len(matches) > 0 {
			err = output.Write70()
			if err != nil {
				panic(err)
			}
		}
	}
	//output.printBin()
}

func (o *output) convertContent(dType string, num string) error {
	// Args will work with the case of FF 41, where there are multiple content values.  It takes the values and splits them,
	// then loops trough the conversion switch.  If the content of the element is a string, splitting it by spaces makes
	// no sense, so splitting is skipped
	args := []string{}
	if dType != "cDeltaString" {
		args = strings.Fields(num)
	} else {
		args = append(args, num)
	}

	for _, arg := range args {
		switch dType {
		case "bool":
			ret := []byte{}
			if arg == "0" {
				ret = []byte("\x00")
			} else {
				ret = []byte("\x01")
			}
			err := binary.Write(o, binary.LittleEndian, ret)
			if err != nil {
				return err
			}
		case "sFloat32":
			flt64, err := strconv.ParseFloat(arg, 32)
			if err != nil {
				return err
			}
			err = binary.Write(o, binary.LittleEndian, float32(flt64))
			if err != nil {
				return err
			}
		case "sInt32":
			newInt, err := strconv.Atoi(arg)
			if err != nil {
				return err
			}
			err = binary.Write(o, binary.LittleEndian, int32(newInt))
			if err != nil {
				return err
			}
		case "sUInt8":
			newInt, err := strconv.Atoi(arg)
			if err != nil {
				return err
			}
			err = binary.Write(o, binary.LittleEndian, uint8(newInt))
			if err != nil {
				return err
			}
		case "cDeltaString":
			// Write the FF designation
			err := binary.Write(o, binary.BigEndian, []byte("\xFF\xFF"))
			if err != nil {
				return err
			}

			// Write the length of the string first
			strLen := len(arg)
			err = binary.Write(o, binary.LittleEndian, int32(strLen))
			if err != nil {
				return err
			}
			// Now write the string
			err = binary.Write(o, binary.BigEndian, []byte(num))
			if err != nil {
				return err
			}
		}
	}
	return nil
}
