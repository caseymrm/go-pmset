package assertions

import (
	"encoding/json"
	"testing"
)

func TestSystemAssertions(t *testing.T) {
	a := GetAssertions()
	b, _ := json.MarshalIndent(a, "", "  ")
	_, ok := a["PreventUserIdleDisplaySleep"]
	if !ok {
		t.Errorf("%s\n", b)
	}
}
