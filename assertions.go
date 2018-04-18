package main

/*
#cgo CFLAGS: -x objective-c
#cgo LDFLAGS: -framework IOKit -framework CoreFoundation

void get_system_assertions();
void get_pid_assertions();
void subscribe_assertions();

*/
import "C"
import (
	"log"
	"sync"
)

// GetAssertions returns a map of assertion keys to it's value
func GetAssertions() map[string]int {
	C.get_system_assertions()
	<-systemDone
	return systemAssertions
}

// GetPIDAssertions returns a map of assertion keys to it's value
func GetPIDAssertions() map[string][]PidAssertion {
	C.get_pid_assertions()
	<-pidDone
	return pidAssertions
}

// SubscribeAssertionChanges hooks up a channel
func SubscribeAssertionChanges() {
	C.subscribe_assertions()
}

// PidAssertion represents one process that has an assertion
type PidAssertion struct {
	PID  int
	Name string
}

var systemMutex = &sync.Mutex{}
var systemAssertions map[string]int
var systemDone = make(chan bool, 1)

var pidMutex = &sync.Mutex{}
var pidAssertions map[string][]PidAssertion
var pidDone = make(chan bool, 1)

//export startSystemAssertions
func startSystemAssertions() {
	systemMutex.Lock()
	systemAssertions = make(map[string]int)
}

//export systemAssertion
func systemAssertion(nameCStr *C.char, val int) {
	name := C.GoString(nameCStr)
	systemAssertions[name] = val
}

//export doneSystemAssertions
func doneSystemAssertions() {
	systemMutex.Unlock()
	systemDone <- true
}

//export startPidAssertions
func startPidAssertions() {
	pidMutex.Lock()
	pidAssertions = make(map[string][]PidAssertion)
}

//export pidAssertion
func pidAssertion(pid int, keyCStr *C.char, val int, nameCStr *C.char, timedoutCStr *C.char) {
	key := C.GoString(keyCStr)
	name := C.GoString(nameCStr)
	timedout := C.GoString(timedoutCStr)
	if timedout != "" {
		log.Printf("Getting pids timed out: %s\n", timedout)
	}
	pidAssertions[key] = append(pidAssertions[key], PidAssertion{
		PID:  pid,
		Name: name,
	})
}

//export donePidAssertions
func donePidAssertions() {
	pidMutex.Unlock()
	pidDone <- true
}

func main() {
	SubscribeAssertionChanges()
}
