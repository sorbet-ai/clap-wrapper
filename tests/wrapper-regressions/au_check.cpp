// au_check COMPONENT: the AUv2 of the wrapper-regressions project at run time. The component is
// registered in this process only (AudioComponentRegister with its bundle's factory), so nothing
// is installed. Checks:
//   - a plugin without the gui extension gives no Cocoa view: kAudioUnitProperty_CocoaUI fails
//     in GetPropertyInfo and in GetProperty;
//   - the class data carries the current preset's name: a user preset set through
//     kAudioUnitProperty_PresentPreset is the kAUPresetNameKey of kAudioUnitProperty_ClassInfo,
//     and restoring class data makes its name the present preset's.
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>

#include <cstdio>
#include <cstring>
#include <string>

static int failures = 0;
static void check(bool ok, const char* what)
{
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
  failures += !ok;
}
static OSType fourcc(CFStringRef s)
{
  char b[8] = {};
  CFStringGetCString(s, b, sizeof b, kCFStringEncodingASCII);
  return (OSType(b[0]) << 24) | (OSType(b[1]) << 16) | (OSType(b[2]) << 8) | OSType(b[3]);
}
static std::string str(CFStringRef s)
{
  char b[256] = {};
  if (s) CFStringGetCString(s, b, sizeof b, kCFStringEncodingUTF8);
  return b;
}

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::fprintf(stderr, "usage: au_check COMPONENT\n");
    return 2;
  }
  CFURLRef url = CFURLCreateFromFileSystemRepresentation(nullptr, reinterpret_cast<const UInt8*>(argv[1]),
                                                         static_cast<CFIndex>(std::strlen(argv[1])), true);
  CFBundleRef bundle = CFBundleCreate(nullptr, url);
  if (!bundle || !CFBundleLoadExecutable(bundle))
  {
    std::fprintf(stderr, "au_check: cannot load %s\n", argv[1]);
    return 2;
  }
  auto components = static_cast<CFArrayRef>(CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("AudioComponents")));
  check(components && CFArrayGetCount(components) == 1, "the bundle's Info.plist has its AudioComponents entry");
  if (!components || CFArrayGetCount(components) < 1) return 1;
  auto info = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(components, 0));
  AudioComponentDescription desc = {fourcc(static_cast<CFStringRef>(CFDictionaryGetValue(info, CFSTR("type")))),
                                    fourcc(static_cast<CFStringRef>(CFDictionaryGetValue(info, CFSTR("subtype")))),
                                    fourcc(static_cast<CFStringRef>(CFDictionaryGetValue(info, CFSTR("manufacturer")))),
                                    0, 0};
  auto factory = reinterpret_cast<AudioComponentFactoryFunction>(CFBundleGetFunctionPointerForName(
      bundle, static_cast<CFStringRef>(CFDictionaryGetValue(info, CFSTR("factoryFunction")))));
  if (!factory)
  {
    std::fprintf(stderr, "au_check: no factory function\n");
    return 2;
  }
  AudioComponent comp = AudioComponentRegister(&desc, CFSTR("Wrapper Regression"), 0x10000, factory);
  AudioUnit unit = nullptr;
  if (!comp || AudioComponentInstanceNew(comp, &unit) != noErr || AudioUnitInitialize(unit) != noErr)
  {
    std::fprintf(stderr, "au_check: cannot open the AU\n");
    return 2;
  }

  // no gui: no Cocoa view
  UInt32 size = 0;
  Boolean writable = false;
  const OSStatus info_rc =
      AudioUnitGetPropertyInfo(unit, kAudioUnitProperty_CocoaUI, kAudioUnitScope_Global, 0, &size, &writable);
  AudioUnitCocoaViewInfo view = {};
  size = sizeof view;
  const OSStatus get_rc = AudioUnitGetProperty(unit, kAudioUnitProperty_CocoaUI, kAudioUnitScope_Global, 0, &view, &size);
  check(info_rc != noErr && get_rc == kAudioUnitErr_InvalidProperty,
        "no gui extension: kAudioUnitProperty_CocoaUI is refused by GetPropertyInfo and GetProperty");

  // the current preset's name in the class data
  AUPreset user = {-1, CFSTR("Regression Preset")};
  check(AudioUnitSetProperty(unit, kAudioUnitProperty_PresentPreset, kAudioUnitScope_Global, 0, &user, sizeof user) == noErr,
        "a user preset set through kAudioUnitProperty_PresentPreset");
  CFPropertyListRef data = nullptr;
  size = sizeof data;
  std::string saved;
  if (AudioUnitGetProperty(unit, kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, &data, &size) == noErr && data)
    saved = str(static_cast<CFStringRef>(CFDictionaryGetValue(static_cast<CFDictionaryRef>(data), CFSTR(kAUPresetNameKey))));
  check(saved == "Regression Preset", ("the class data's name is the present preset's (\"" + saved + "\")").c_str());

  // restoring class data makes its name the present preset's
  if (data)
  {
    CFMutableDictionaryRef copy = CFDictionaryCreateMutableCopy(nullptr, 0, static_cast<CFDictionaryRef>(data));
    CFDictionarySetValue(copy, CFSTR(kAUPresetNameKey), CFSTR("Restored"));
    CFPropertyListRef in = copy;
    check(AudioUnitSetProperty(unit, kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, &in, sizeof in) == noErr,
          "the class data restored");
    AUPreset present = {};
    size = sizeof present;
    std::string name;
    if (AudioUnitGetProperty(unit, kAudioUnitProperty_PresentPreset, kAudioUnitScope_Global, 0, &present, &size) == noErr)
    {
      name = str(present.presetName);
      if (present.presetName) CFRelease(present.presetName);
    }
    check(name == "Restored", ("the restored class data's name is the present preset's (\"" + name + "\")").c_str());
    CFRelease(copy);
    CFRelease(data);
  }

  AudioUnitUninitialize(unit);
  AudioComponentInstanceDispose(unit);
  std::printf(failures ? "au_check: %d FAILED\n" : "au_check: all passed\n", failures);
  return failures ? 1 : 0;
}
