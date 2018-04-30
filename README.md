# go-assertions
Golang library to access OSX's system assertions, the ones displayed when you run `pmset -g assertions`

## Installation
go-assertions requires OS X.

`go get github.com/caseymrm/go-assertions`

## Documentation

https://godoc.org/github.com/caseymrm/go-assertions

## Examples

### GetAssertions()

```go
package main

import (
    "encoding/json"
    "log"
    
    "github.com/caseymrm/go-assertions"
)

func main() {
    a := assertions.GetAssertions()
    b, _ := json.MarshalIndent(a, "", "  ")
    log.Printf("%s\n", b)
}
```

```
2018/04/17 16:11:23 
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

### GetPIDAssertions()

```go
package main

import (
    "encoding/json"
    "log"
    
    "github.com/caseymrm/go-assertions"
)

func main() {
    a := assertions.GetPIDAssertions()
    b, _ := json.MarshalIndent(a, "", "  ")
    log.Printf("%s\n", b)
}
```

```
2018/04/17 16:11:23 {
  "PreventUserIdleDisplaySleep": [
    {
      "PID": 47784,
      "Name": "com.apple.WebCore: HTMLMediaElement playback"
    }
  ],
  "PreventUserIdleSystemSleep": [
    {
      "PID": 180,
      "Name": "com.apple.audio.AppleUSBAudioEngine:C-Media Electronics Inc.:USB Audio Device:14131000:2,1.context.preventuseridlesleep"
    },
    {
      "PID": 180,
      "Name": "com.apple.audio.AppleUSBAudioEngine:C-Media Electronics Inc.:USB Audio Device:14131000:2,1.context.preventuseridlesleep"
    }
  ],
  "UserIsActive": [
    {
      "PID": 114,
      "Name": "com.apple.iohideventsystem.queue.tickle.4294978958.17"
    }
  ]
}
```


### SubscribeAssertionChanges()
### SubscribeAssertionChangesAndRun()

```go
package main

import (
    "encoding/json"
    "log"
    
    "github.com/caseymrm/go-assertions"
)

func main() {
	channel := make(chan AssertionChange)
	go func() {
		for change := range channel {
            b, _ := json.MarshalIndent(change, "", "  ")
            log.Printf("%s\n", b)
		}
	}()
	SubscribeAssertionChangesAndRun(channel)
}
```

```
2018/04/30 11:30:06 {
  "Action": "Released",
  "Type": "PreventUserIdleDisplaySleep",
  "Pid": {
    "PID": 47784,
    "Name": "com.apple.WebCore: HTMLMediaElement playback"
  }
}
2018/04/30 11:30:07 {
  "Action": "Created",
  "Type": "PreventUserIdleDisplaySleep",
  "Pid": {
    "PID": 47784,
    "Name": "com.apple.WebCore: HTMLMediaElement playback"
  }
}
```
## License

MIT
