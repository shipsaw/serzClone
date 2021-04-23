package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"regexp"
	"strings"
)

var cpuprofile = flag.String("cpuprofile", "", "write cpu profile to `file`")
var memprofile = flag.String("memprofile", "", "write memory profile to `file`")

//var allPatt string = `<(/)?([\w]+)(/)?>?(?: d:id="([^"]+)")?(?: d:numElements="([^"]+)")?(?: d:type="([^"]+)")?(?: d:alt[^ ]+)?(?: d:elementType="([^"]+)")?(?: d:precision[^>]+)?>?([^<]+)`
var allPatt string = `<([\w]+)(?: xm[^ ]+)(?: d:ver[^ ]+)(?: d:id="([^"]+)")`

func main() {
	//defer profile.Start().Stop()

	allPatt := regexp.MustCompile(allPatt)

	/*
		//symbolMap := new(symbolMap)
			if len(os.Args) <= 1 {
				fmt.Printf("USAGE : %s <target_filename> \n", os.Args[0])
				os.Exit(0)
			}

			fileName := os.Args[1]
	*/

	fileBytes, err := ioutil.ReadFile("oldTest.xml")
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}

	sliceData := strings.Split(string(fileBytes), "\n")
	for i, slice := range sliceData {
		fmt.Printf("%v  ", i)
		matches := allPatt.FindStringSubmatch(slice)
		for i, _ := range matches {
			fmt.Printf("%v   ", matches[i])
		}
		fmt.Println("")
	}
	//output.printBin()
}
