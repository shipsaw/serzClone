package main

import (
	"encoding/binary"
	"fmt"
	"os"
)

func main() {
	test1 := 1
	test2 := 2
	test3 := 3
	err := binary.Write(os.Stdout, binary.LittleEndian, int32(test1))
	if err != nil {
		panic(err)
	}
	fmt.Println(test1, test2, test3)
}
