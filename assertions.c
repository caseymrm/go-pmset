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

void startSystemAssertions();
void systemAssertion(const char *, int);
void doneSystemAssertions();

void startPidAssertions();
void pidAssertion(int, const char *, int, const char *, const char *);
void donePidAssertions();

void get_system_assertions() {
  startSystemAssertions();

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
    doneSystemAssertions();
    return;
  }

  count = CFDictionaryGetCount(assertions_status);
  if (0 == count) {
    doneSystemAssertions();
    return;
  }

  assertionNames = (CFStringRef *)malloc(sizeof(CFStringRef *) * count);
  assertionValues = (CFNumberRef *)malloc(sizeof(CFNumberRef *) * count);
  CFDictionaryGetKeysAndValues(assertions_status, (const void **)assertionNames,
                               (const void **)assertionValues);

  for (i = 0; i < count; i++) {
    CFStringGetCString(assertionNames[i], name, 50, kCFStringEncodingMacRoman);
    CFNumberGetValue(assertionValues[i], kCFNumberIntType, &val);
    systemAssertion(name, val);
  }

  free(assertionNames);
  free(assertionValues);
  doneSystemAssertions();
}

#define kIOPMAssertionTimedOutDateKey CFSTR("AssertTimedOutWhen")

void get_pid_assertions() {
  startPidAssertions();

  CFDictionaryRef assertions_info = NULL;

  IOReturn ret = IOPMCopyAssertionsByProcess(&assertions_info);
  if (kIOReturnSuccess != ret) {
    donePidAssertions();
    return;
  }

  if (assertions_info) {
    CFNumberRef *pids = NULL;
    CFArrayRef *assertions = NULL;
    int process_count;
    int i;

    process_count = CFDictionaryGetCount(assertions_info);
    pids = malloc(sizeof(CFNumberRef) * process_count);
    assertions = malloc(sizeof(CFArrayRef *) * process_count);
    CFDictionaryGetKeysAndValues(assertions_info, (const void **)pids,
                                 (const void **)assertions);

    for (i = 0; i < process_count; i++) {
      int the_pid;
      int j;

      CFNumberGetValue(pids[i], kCFNumberIntType, &the_pid);

      for (j = 0; j < CFArrayGetCount(assertions[i]); j++) {
        CFDictionaryRef tmp_dict;
        CFStringRef tmp_type;
        CFNumberRef tmp_val;
        CFStringRef the_name;
        char str_buf[40];
        char name_buf[129];
        int val;
        bool timed_out = false;

        tmp_dict = CFArrayGetValueAtIndex(assertions[i], j);
        if (!tmp_dict) {
          return;
        }
        tmp_type = CFDictionaryGetValue(tmp_dict, kIOPMAssertionTypeKey);
        tmp_val = CFDictionaryGetValue(tmp_dict, kIOPMAssertionLevelKey);
        the_name = CFDictionaryGetValue(tmp_dict, kIOPMAssertionNameKey);
        timed_out =
            CFDictionaryGetValue(tmp_dict, kIOPMAssertionTimedOutDateKey);
        if (!tmp_type || !tmp_val) {
          return;
        }
        CFStringGetCString(tmp_type, str_buf, 40, kCFStringEncodingMacRoman);
        CFNumberGetValue(tmp_val, kCFNumberIntType, &val);

        if (the_name) {
          CFStringGetCString(the_name, name_buf, 129,
                             kCFStringEncodingMacRoman);
        }

        pidAssertion(the_pid, str_buf, val,
                     the_name ? name_buf : "zilch-o (bug!)",
                     timed_out ? "timed out :(" : "");
      }
    }
  } else {
    printf("No Assertions.\n");
  }
  if (assertions_info)
    CFRelease(assertions_info);

  donePidAssertions();
}