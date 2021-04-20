package main

import (
	"fmt"
	"io/ioutil"
	"os"
	"regexp"
	"strconv"
	"strings"
)

type line struct {
	LineType  int
	Id        uint32
	NameLen   uint32
	Name      []byte
	TypeLen   uint32
	Type      []byte
	ContCount uint8
	Content   interface{}
	RetLine   uint16
}

type symbolMap map[string]uint16

type symbolLocation map[string]uint16

type structSlice []line

var ff50patt string = `<(\w+)(?:.*?d:id="(\d{7,10})")?`
var ff56patt string = `<(\w+) (?:d:type="(\w+?)".*?>(.+?))<`
var ff41patt string = `<(\w+) d:numElements="(\d+)" d:elementType="(\w+)" .+?>(.+?)<`
var ff70patt string = `</(\w+?)>`

func main() {
	ff50regxp := regexp.MustCompile(ff50patt)
	ff56regxp := regexp.MustCompile(ff56patt)
	ff41regxp := regexp.MustCompile(ff41patt)
	ff70regxp := regexp.MustCompile(ff70patt)

	/*
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
	lineSlice := make([]line, len(sliceData)-1) // -1 to not include the header line in the xml doc

	for i, slice := range sliceData {
		linetype := 0
		matches := ff41regxp.FindStringSubmatch(slice)
		if len(matches) > 0 {
			linetype = 41
			lineSlice[i].fillStruct(matches[1:], linetype)
			continue
		}
		matches = ff56regxp.FindStringSubmatch(slice)
		if len(matches) > 0 {
			linetype = 56
			lineSlice[i].fillStruct(matches[1:], linetype)
			continue
		}
		matches = ff50regxp.FindStringSubmatch(slice)
		if len(matches) > 0 {
			linetype = 50
			lineSlice[i].fillStruct(matches[1:], linetype)
			continue
		}
		matches = ff70regxp.FindStringSubmatch(slice)
		if len(matches) > 0 {
			linetype = 70
			lineSlice[i].fillStruct(matches[1:], linetype)
		}
	}
}

func (l *line) fillStruct(matches []string, linetype int) {
	switch linetype {
	case 41:
		l.NameLen = uint32(len(matches[0]))
		l.Name = []byte(matches[0])
		l.TypeLen = uint32(len(matches[1]))
		l.Type = []byte(matches[1])
		l.ContCount = uint8(len(matches[2]))
		// l.Content =
	case 56:
		l.NameLen = uint32(len(matches[0]))
		l.Name = []byte(matches[0])
		l.TypeLen = uint32(len(matches[1]))
		l.Type = []byte(matches[1])
		// l.Content =
	case 50:
		l.NameLen = uint32(len(matches[0]))
		l.Name = []byte(matches[0])
		tempId, _ := strconv.Atoi(matches[1])
		l.Id = uint32(tempId)
	case 70:
		// TODO
	}
}
