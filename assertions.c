#include <CoreFoundation/CFDateFormatter.h>
#include <CoreFoundation/CoreFoundation.h>

#include <IOKit/IOCFSerialize.h>
#include <IOKit/IOMessage.h>
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/pwr_mgt/IOPM.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <sys/sysctl.h>
#include <sys/types.h>

#include <asl.h>
#include <ctype.h>
#include <dirent.h>
#include <libproc.h>
#include <mach/mach_port.h>
#include <notify.h>
#include <servers/bootstrap.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

void startAssertions();
void assertion(const char *, int);
void doneAssertions();

void get_system_assertions() {
  startAssertions();

  CFStringRef *assertionNames = NULL;
  CFNumberRef *assertionValues = NULL;
  char name[50];
  int val;
  CFIndex count;
  CFDictionaryRef assertions_status;
  IOReturn ret;
  int i;

  ret = IOPMCopyAssertionsStatus(&assertions_status);
  if ((kIOReturnSuccess != ret) || (NULL == assertions_status)) {
    printf("No assertions.\n");
    return;
  }

  count = CFDictionaryGetCount(assertions_status);
  if (0 == count) {
    return;
  }

  assertionNames = (CFStringRef *)malloc(sizeof(CFStringRef *) * count);
  assertionValues = (CFNumberRef *)malloc(sizeof(CFNumberRef *) * count);
  CFDictionaryGetKeysAndValues(assertions_status, (const void **)assertionNames,
                               (const void **)assertionValues);

  for (i = 0; i < count; i++) {
    CFStringGetCString(assertionNames[i], name, 50, kCFStringEncodingMacRoman);
    CFNumberGetValue(assertionValues[i], kCFNumberIntType, &val);
    assertion(name, val);
  }

  free(assertionNames);
  free(assertionValues);
  doneAssertions();
}

static void logAssertionsCallBack(CFMachPortRef port __unused,
                                  void *msg __unused, CFIndex size __unused,
                                  void *info __unused) {
  get_system_assertions();
}

#define kIOPMAssertionsChangedNotifyString                                     \
  "com.apple.system.powermanagement.assertions"
#define kIOPMAssertionTimedOutNotifyString                                     \
  "com.apple.system.powermanagement.assertions.timeout"
#define kIOPMAssertionsAnyChangedNotifyString                                  \
  "com.apple.system.powermanagement.assertions.anychange"
enum {
    kIOPMNotifyRegister = 0x1,
    kIOPMNotifyDeRegister = 0x2
};
