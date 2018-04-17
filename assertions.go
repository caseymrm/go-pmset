package assertions

/*
#cgo CFLAGS: -x objective-c
#cgo LDFLAGS: -framework IOKit -framework CoreFoundation

void get_system_assertions();

*/
import "C"
import (
	"sync"
)

// GetAssertions returns a map of assertion keys to it's value
func GetAssertions() map[string]int {
	doneChan = make(chan bool, 1)
	C.get_system_assertions()
	<-doneChan
	return assertions
}

var mutex = &sync.Mutex{}
var assertions map[string]int
var doneChan chan bool

//export startAssertions
func startAssertions() {
	mutex.Lock()
	assertions = make(map[string]int)
}

//export assertion
func assertion(nameCStr *C.char, val int) {
	name := C.GoString(nameCStr)
	assertions[name] = val
}

//export doneAssertions
func doneAssertions() {
	mutex.Unlock()
	doneChan <- true
}
