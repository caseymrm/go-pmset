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

#include "IOPMLibPrivate.h"
#include "isacf.h"

void startSystemAssertions();
void systemAssertion(const char *, int);
void doneSystemAssertions();

void startPidAssertions();
void pidAssertion(int, const char *, int, const char *, const char *);
void donePidAssertions();

void assertionChangeStart();
void subscriptionAction(const char *);
void subscriptionType(const char *);
void subscriptionPid(int);
void subscriptionProcessName(const char *);
void assertionChangeReady();

void startThermConditions();
void thermCondition(const char *, int);
void doneThermConditions();

void get_system_assertions()
{
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
  if ((kIOReturnSuccess != ret) || (NULL == assertions_status))
  {
    printf("No assertions.\n");
    doneSystemAssertions();
    return;
  }

  count = CFDictionaryGetCount(assertions_status);
  if (0 == count)
  {
    doneSystemAssertions();
    return;
  }

  assertionNames = (CFStringRef *)malloc(sizeof(CFStringRef *) * count);
  assertionValues = (CFNumberRef *)malloc(sizeof(CFNumberRef *) * count);
  CFDictionaryGetKeysAndValues(assertions_status, (const void **)assertionNames,
                               (const void **)assertionValues);

  for (i = 0; i < count; i++)
  {
    CFStringGetCString(assertionNames[i], name, 50, kCFStringEncodingMacRoman);
    CFNumberGetValue(assertionValues[i], kCFNumberIntType, &val);
    systemAssertion(name, val);
  }

  free(assertionNames);
  free(assertionValues);
  doneSystemAssertions();
}

#define kIOPMAssertionTimedOutDateKey CFSTR("AssertTimedOutWhen")

void get_pid_assertions()
{
  startPidAssertions();

  CFDictionaryRef assertions_info = NULL;

  IOReturn ret = IOPMCopyAssertionsByProcess(&assertions_info);
  if (kIOReturnSuccess != ret)
  {
    donePidAssertions();
    return;
  }

  if (!assertions_info)
  {
    printf("No Assertions.\n");
    donePidAssertions();
    return;
  }

  CFNumberRef *pids = NULL;
  CFArrayRef *assertions = NULL;
  int process_count;
  int i;

  process_count = CFDictionaryGetCount(assertions_info);
  pids = malloc(sizeof(CFNumberRef) * process_count);
  assertions = malloc(sizeof(CFArrayRef *) * process_count);
  CFDictionaryGetKeysAndValues(assertions_info, (const void **)pids,
                               (const void **)assertions);

  for (i = 0; i < process_count; i++)
  {
    int the_pid;
    int j;

    CFNumberGetValue(pids[i], kCFNumberIntType, &the_pid);

    for (j = 0; j < CFArrayGetCount(assertions[i]); j++)
    {
      CFDictionaryRef tmp_dict;
      CFStringRef tmp_type;
      CFNumberRef tmp_val;
      CFStringRef the_name;
      char str_buf[40];
      char name_buf[129];
      int val;
      bool timed_out = false;

      tmp_dict = CFArrayGetValueAtIndex(assertions[i], j);
      if (!tmp_dict)
      {
        donePidAssertions();
        return;
      }
      tmp_type = CFDictionaryGetValue(tmp_dict, kIOPMAssertionTypeKey);
      tmp_val = CFDictionaryGetValue(tmp_dict, kIOPMAssertionLevelKey);
      the_name = CFDictionaryGetValue(tmp_dict, kIOPMAssertionNameKey);
      timed_out = CFDictionaryGetValue(tmp_dict, kIOPMAssertionTimedOutDateKey);
      if (!tmp_type || !tmp_val)
      {
        return;
      }
      CFStringGetCString(tmp_type, str_buf, 40, kCFStringEncodingMacRoman);
      CFNumberGetValue(tmp_val, kCFNumberIntType, &val);

      if (the_name)
      {
        CFStringGetCString(the_name, name_buf, 129, kCFStringEncodingMacRoman);
      }

      pidAssertion(the_pid, str_buf, val,
                   the_name ? name_buf : "zilch-o (bug!)",
                   timed_out ? "timed out :(" : "");
    }
  }
  if (assertions_info)
    CFRelease(assertions_info);

  donePidAssertions();
}

static void get_new_activity()
{
  int num;
  CFIndex cnt = 0;
  char str[200];
  bool of;
  uint64_t num64;
  CFArrayRef log = NULL;
  CFNumberRef num_cf = NULL;
  CFStringRef str_cf = NULL;
  static uint32_t refCnt = UINT_MAX;
  CFDictionaryRef entry;
  IOReturn rc;
  pid_t beneficiary;

  rc = IOPMCopyAssertionActivityUpdate(&log, &of, &refCnt);
  if ((rc != kIOReturnSuccess) && (rc != kIOReturnNotFound))
  {
    // TODO: some kind of error
    return;
  }
  if (!log)
  {
    return;
  }
  if (of)
  {
    // TODO: indicate overflow, may have missed some
  }
  cnt = isA_CFArray(log) ? CFArrayGetCount(log) : 0;
  for (int i = 0; i < cnt; i++)
  {
    entry = CFArrayGetValueAtIndex(log, i);
    if (entry == NULL)
      continue;

    assertionChangeStart();
    str_cf = CFDictionaryGetValue(entry, kIOPMAssertionActivityAction);
    str[0] = 0;
    if (isA_CFString(str_cf))
    {
      CFStringGetCString(str_cf, str, sizeof(str), kCFStringEncodingMacRoman);
      subscriptionAction(str);
    }

    str_cf = CFDictionaryGetValue(entry, kIOPMAssertionTypeKey);
    str[0] = 0;
    if (isA_CFString(str_cf))
    {
      CFStringGetCString(str_cf, str, sizeof(str), kCFStringEncodingMacRoman);
      subscriptionType(str);
    }

    num_cf = CFDictionaryGetValue(entry, kIOPMAssertionPIDKey);
    if (isA_CFNumber(num_cf))
    {
      CFNumberGetValue(num_cf, kCFNumberIntType, &num);
      subscriptionPid(num);

      num_cf = CFDictionaryGetValue(entry, kIOPMAssertionOnBehalfOfPID);
      if (isA_CFNumber(num_cf))
      {
        CFNumberGetValue(num_cf, kCFNumberIntType, &beneficiary);
        // TODO: expose beneficiary to go
      }
    }

    str_cf = CFDictionaryGetValue(entry, kIOPMAssertionNameKey);
    str[0] = 0;
    if (isA_CFString(str_cf))
    {
      CFStringGetCString(str_cf, str, sizeof(str), kCFStringEncodingMacRoman);
      subscriptionProcessName(str);
    }

    assertionChangeReady();
  }

exit:

  if (cnt)
    CFRelease(log);
}

void subscribe_assertions()
{
  int token;
  int notify_status;

  IOPMAssertionNotify(kIOPMAssertionsAnyChangedNotifyString,
                      kIOPMNotifyRegister);
  IOPMSetAssertionActivityLog(true);
  notify_status =
      notify_register_dispatch(kIOPMAssertionsAnyChangedNotifyString, &token,
                               dispatch_get_main_queue(), ^(int t) {
                                 get_new_activity();
                               });

  if (NOTIFY_STATUS_OK != notify_status)
  {
    printf("Could not get notification for %s. Exiting.\n",
           kIOPMAssertionsAnyChangedNotifyString);
    return;
  }
}

void run_subscribed_assertions() { dispatch_main(); }

void get_thermal_conditions()
{
  CFDictionaryRef cpuStatus;
  CFStringRef *keys = NULL;
  CFNumberRef *vals = NULL;
  CFIndex count = 0;
  int i;
  IOReturn ret;

  startThermConditions();

  ret = IOPMCopyCPUPowerStatus(&cpuStatus);

  if (kIOReturnNotFound == ret)
  {
    printf("Note: No CPU power status has been recorded\n");
    return;
  }

  if (!cpuStatus || (kIOReturnSuccess != ret))
  {
    printf("Error: No CPU power status with error code 0x%08x\n", ret);
    return;
  }

  count = CFDictionaryGetCount(cpuStatus);

  keys = (CFStringRef *)malloc(count * sizeof(CFStringRef));
  vals = (CFNumberRef *)malloc(count * sizeof(CFNumberRef));
  if (!keys || !vals)
  {
    goto exit;
  }

  CFDictionaryGetKeysAndValues(cpuStatus,
                               (const void **)keys, (const void **)vals);
  for (i = 0; i < count; i++)
  {
    char strbuf[125];
    int valint;

    CFStringGetCString(keys[i], strbuf, 125, kCFStringEncodingUTF8);
    CFNumberGetValue(vals[i], kCFNumberIntType, &valint);
    thermCondition(strbuf, valint);
  }

exit:
  if (keys)
    free(keys);
  if (vals)
    free(vals);
  if (cpuStatus)
    CFRelease(cpuStatus);
  doneThermConditions();
}