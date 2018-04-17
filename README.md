# go-assertions
Golang library to access OSX's system assertions, the ones displayed when you run `pmset -g assertions`

## Installation
go-assertions requires OS X.

`go get github.com/caseymrm/go-assertions`

## Hello World

```go
package main

import (
    "encoding/json"
    "log"
    
    "github.com/caseymrm/go-assertions"
)

func main() {
	a := GetAssertions()
	b, _ := json.MarshalIndent(a, "", "  ")
	log.Printf("%s\n", b)
}
```

```
2018/04/17 16:11:23 Hello World
{
  "ApplePushServiceTask": 0,
  "AwakeOnReservePower": 0,
  "BackgroundTask": 0,
  "CPUBoundAssertion": 0,
  "ChargeInhibit": 0,
  "DisableInflow": 0,
  "DisableLowPowerBatteryWarnings": 0,
  "DisplayWake": 0,
  "EnableIdleSleep": 1,
  "ExternalMedia": 0,
  "InteractivePushServiceTask": 0,
  "InternalPreventDisplaySleep": 1,
  "InternalPreventSleep": 0,
  "NetworkClientActive": 0,
  "PreventDiskIdle": 0,
  "PreventSystemSleep": 0,
  "PreventUserIdleDisplaySleep": 1,
  "PreventUserIdleSystemSleep": 1,
  "SystemIsActive": 0,
  "UserIsActive": 1
}
```

## Documentation

https://godoc.org/github.com/caseymrm/go-assertions

## TODO
1. Assertions by pid
2. `pmset -g assertionslog` behavior

## License

MIT
