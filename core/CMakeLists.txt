set(HEADER_FILES
    atomic.h
    base64.h
    command_line.h
    defer.h
    defines.h
    ecs.h
    formatter.h
    hash.h
    json.h
    log.h
    print.h
    reflect.h
    result.h
    scheduler.h
    source_location.h
    tester.h
    compiler/compiler_clang.h
    compiler/compiler_gcc.h
    compiler/compiler_msvc.h
    compiler/compiler.h
    containers/array.h
    containers/hash_set.h
    containers/hash_table.h
    containers/ring_buffer.h
    containers/slice.h
    containers/stack_array.h
    containers/string_interner.h
    containers/string.h
    math/f32.h
    math/f64.h
    math/i32.h
    math/i64.h
    math/u32.h
    math/u64.h
    math/f32x2.h
    math/f32x3.h
    math/f32x4.h
    math/f64x2.h
    math/f64x3.h
    math/f64x4.h
    math/i32x2.h
    math/i32x3.h
    math/i32x4.h
    math/u32x2.h
    math/u32x3.h
    math/u32x4.h
    math/f32x2x2.h
    math/f32x3x3.h
    math/f32x4x4.h
    math/f64x2x2.h
    math/f64x3x3.h
    math/f64x4x4.h
    math/quaternion.h
    math/random.h
    memory/allocator.h
    memory/heap_allocator.h
    memory/arena_allocator.h
    memory/pool_allocator.h
    platform/platform.h
    serialization/binary_serializer.h
    serialization/json_serializer.h
    validate.h
)

set(SOURCE_FILES
    containers/string_interner.cpp
    base64.cpp
    command_line.cpp
    ecs.cpp
    json.cpp
    scheduler.cpp
    tester.cpp
    memory/heap_allocator.cpp
    memory/arena_allocator.cpp
    memory/pool_allocator.cpp
    validate.cpp
)

set(LIBS
    core-options
)

set(TARGET_PROPERTIES
    UNITY_BUILD_BATCH_SIZE 8
    UNITY_BUILD ${CORE_BUILD_UNITY}
)

if(WIN32)
    list(APPEND SOURCE_FILES
        platform/platform_win32.cpp
    )
    list(APPEND LIBS
        user32.lib
        winmm.lib
        shell32.lib
        ole32.lib
        dbghelp
    )
elseif(LINUX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)

    list(APPEND SOURCE_FILES
        platform/platform_linux.cpp
    )
    list(APPEND LIBS
        xcb
        X11
        X11-xcb
        PkgConfig::DBUS
        dl
        pthread
    )
elseif(APPLE)
    add_library(core-platform-macos OBJECT platform/platform_macos.mm)
    target_link_libraries(core-platform-macos PRIVATE ${LIBS} "-framework Cocoa" "-framework IOKit" "-framework UniformTypeIdentifiers")

    list(APPEND LIBS
        core-platform-macos
    )
elseif(ANDROID)
    list(APPEND SOURCE_FILES
        platform/platform_android.cpp
    )
    list(APPEND LIBS
        android
        dl
    )

    set(CORE_ANDROID_JAVA_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/android/java")
    set(CORE_ANDROID_JAVA_PACKAGE_DIR "${CORE_ANDROID_JAVA_SOURCE_DIR}/core/android")
    set(CORE_ANDROID_JAVA_SOURCE "${CORE_ANDROID_JAVA_PACKAGE_DIR}/CoreNativeActivity.java")
    set(CORE_ANDROID_CLIPBOARD_PROVIDER_JAVA_SOURCE "${CORE_ANDROID_JAVA_PACKAGE_DIR}/CoreClipboardProvider.java")

    file(MAKE_DIRECTORY "${CORE_ANDROID_JAVA_PACKAGE_DIR}")
    set(CORE_ANDROID_JAVA_SOURCE_CONTENT [=[
        |package core.android;
        |
        |import android.app.NativeActivity;
        |import android.content.pm.ActivityInfo;
        |import android.content.pm.PackageManager;
        |import android.content.ClipData;
        |import android.content.ClipDescription;
        |import android.content.ClipboardManager;
        |import android.content.ComponentCallbacks2;
        |import android.content.Context;
        |import android.content.Intent;
        |import android.database.Cursor;
        |import android.graphics.Color;
        |import android.graphics.Rect;
        |import android.net.Uri;
        |import android.os.Build;
        |import android.os.Bundle;
        |import android.os.ParcelFileDescriptor;
        |import android.provider.DocumentsContract;
        |import android.provider.OpenableColumns;
        |import android.text.InputType;
        |import android.view.KeyEvent;
        |import android.view.View;
        |import android.view.ViewGroup;
        |import android.view.Window;
        |import android.view.WindowManager;
        |import android.view.inputmethod.BaseInputConnection;
        |import android.view.inputmethod.EditorInfo;
        |import android.view.inputmethod.ExtractedText;
        |import android.view.inputmethod.ExtractedTextRequest;
        |import android.view.inputmethod.InputConnection;
        |import android.view.inputmethod.InputMethodManager;
        |import android.webkit.MimeTypeMap;
        |
        |import java.io.ByteArrayOutputStream;
        |import java.io.File;
        |import java.io.FileOutputStream;
        |import java.io.InputStream;
        |import java.nio.charset.StandardCharsets;
        |import java.security.SecureRandom;
        |import java.util.List;
        |import java.util.Locale;
        |
        |public class CoreNativeActivity extends NativeActivity {
        |    private static final int CORE_REQUEST_OPEN_DOCUMENT = 0x434f5245;
        |    private static final int CORE_REQUEST_CREATE_DOCUMENT = 0x434f5246;
        |    private static final int CORE_REQUEST_OPEN_TREE = 0x434f5247;
        |    private static final int CORE_TEXT_INPUT_ACTION_NONE = 0;
        |    private static final int CORE_TEXT_INPUT_ACTION_DONE = 1;
        |    private static final int CORE_TEXT_INPUT_ACTION_GO = 2;
        |    private static final int CORE_TEXT_INPUT_ACTION_SEARCH = 3;
        |    private static final int CORE_TEXT_INPUT_ACTION_SEND = 4;
        |    private static final int CORE_TEXT_INPUT_ACTION_NEXT = 5;
        |    private static final int CORE_TEXT_INPUT_ACTION_PREVIOUS = 6;
        |    private static final int CORE_TEXT_INPUT_FLAG_MULTILINE = 1 << 0;
        |    private static final int CORE_TEXT_INPUT_FLAG_PASSWORD = 1 << 1;
        |    private static final int CORE_TEXT_INPUT_FLAG_NUMBER = 1 << 2;
        |    private static final int CORE_TEXT_INPUT_FLAG_DECIMAL = 1 << 3;
        |    private static final int CORE_TEXT_INPUT_FLAG_SIGNED = 1 << 4;
        |    private static final int CORE_TEXT_INPUT_FLAG_EMAIL = 1 << 5;
        |    private static final int CORE_TEXT_INPUT_FLAG_URI = 1 << 6;
        |    private static final int CORE_TEXT_INPUT_FLAG_NO_SUGGESTIONS = 1 << 7;
        |    private static final int CORE_WINDOW_PRESENTATION_FLAG_FULLSCREEN = 1 << 0;
        |    private static final int CORE_WINDOW_PRESENTATION_FLAG_IMMERSIVE = 1 << 1;
        |    private static final int CORE_WINDOW_PRESENTATION_FLAG_KEEP_SCREEN_ON = 1 << 2;
        |    private static final int CORE_WINDOW_PRESENTATION_FLAG_EDGE_TO_EDGE = 1 << 3;
        |    private static final int CORE_WINDOW_ORIENTATION_POLICY_SYSTEM = 0;
        |    private static final int CORE_WINDOW_ORIENTATION_POLICY_PORTRAIT = 1;
        |    private static final int CORE_WINDOW_ORIENTATION_POLICY_LANDSCAPE = 2;
        |    private static final int CORE_WINDOW_ORIENTATION_POLICY_SENSOR = 3;
        |    private static final int CORE_WINDOW_ORIENTATION_POLICY_SENSOR_PORTRAIT = 4;
        |    private static final int CORE_WINDOW_ORIENTATION_POLICY_SENSOR_LANDSCAPE = 5;
        |
        |    private long coreFileDialogRequest;
        |    private CoreTextInputView coreTextInputView;
        |    private boolean coreWindowPresentationCaptured;
        |    private int coreWindowPresentationFlags;
        |    private int coreWindowOrientationPolicy;
        |    private int coreWindowSystemUiVisibility;
        |    private int coreWindowRequestedOrientation;
        |    private int coreWindowStatusBarColor;
        |    private int coreWindowNavigationBarColor;
        |    private int coreWindowCutoutMode;
        |    private boolean coreNativeLibraryLoaded;
        |
        |    private static native void nativeActivityDestroying(boolean changingConfigurations, boolean finishing);
        |    private static native void nativeMemoryPressure();
        |    private static native void nativeFileDialogResult(long requestId, boolean accepted, String uri);
        |    private static native void nativeTextInputCommit(byte[] text);
        |    private static native void nativeTextInputCompose(byte[] text);
        |    private static native void nativeTextInputComposeEnd();
        |    private static native void nativeTextInputComposeRegion(int start, int end);
        |    private static native void nativeTextInputDelete(int beforeLength, int afterLength);
        |    private static native void nativeTextInputSelection(int start, int end);
        |    private static native void nativeTextInputAction(int action);
        |
        |    private void coreLoadNativeLibrary() {
        |        if (coreNativeLibraryLoaded) {
        |            return;
        |        }
        |
        |        ActivityInfo activityInfo;
        |        try {
        |            activityInfo = getPackageManager().getActivityInfo(getComponentName(), PackageManager.GET_META_DATA);
        |        } catch (PackageManager.NameNotFoundException exception) {
        |            throw new UnsatisfiedLinkError("Core NativeActivity metadata could not be read: " + exception.getMessage());
        |        }
        |
        |        if (activityInfo.metaData == null) {
        |            throw new UnsatisfiedLinkError("Core NativeActivity requires android.app.lib_name metadata.");
        |        }
        |
        |        String libraryName = activityInfo.metaData.getString("android.app.lib_name");
        |        if (libraryName == null || libraryName.length() == 0) {
        |            throw new UnsatisfiedLinkError("Core NativeActivity requires a non-empty android.app.lib_name metadata value.");
        |        }
        |
        |        System.loadLibrary(libraryName);
        |        coreNativeLibraryLoaded = true;
        |    }
        |
        |    private static String coreCleanExtension(String extension) {
        |        if (extension == null) {
        |            return "";
        |        }
        |
        |        int begin = 0;
        |        while (begin < extension.length()) {
        |            char c = extension.charAt(begin);
        |            if (c != '*' && c != '.' && c != ' ') {
        |                break;
        |            }
        |            ++begin;
        |        }
        |
        |        int end = begin;
        |        while (end < extension.length()) {
        |            char c = extension.charAt(end);
        |            if (c == ';' || c == ',' || c == ' ') {
        |                break;
        |            }
        |            ++end;
        |        }
        |
        |        return extension.substring(begin, end);
        |    }
        |
        |    private static String coreMimeType(String extension) {
        |        String cleanExtension = coreCleanExtension(extension);
        |        if (cleanExtension.length() == 0) {
        |            return "*/*";
        |        }
        |
        |        String mimeType = MimeTypeMap.getSingleton().getMimeTypeFromExtension(cleanExtension.toLowerCase(Locale.ROOT));
        |        return mimeType != null ? mimeType : "*/*";
        |    }
        |
        |    private static String coreDefaultName(String extension) {
        |        String cleanExtension = coreCleanExtension(extension);
        |        return cleanExtension.length() > 0 ? "untitled." + cleanExtension : "untitled";
        |    }
        |
        |    private static String coreMimeTypeFromName(String name) {
        |        if (name == null) {
        |            return "*/*";
        |        }
        |
        |        int dot = name.lastIndexOf('.');
        |        if (dot < 0 || dot + 1 >= name.length()) {
        |            return "*/*";
        }
        |
        |        String mimeType = MimeTypeMap.getSingleton().getMimeTypeFromExtension(name.substring(dot + 1).toLowerCase(Locale.ROOT));
        |        return mimeType != null ? mimeType : "*/*";
        |    }
        |
        |    private static boolean coreExtensionMatches(String name, String extension) {
        |        String cleanExtension = coreCleanExtension(extension);
        |        if (cleanExtension.length() == 0) {
        |            return true;
        |        }
        |
        |        return name != null && name.toLowerCase(Locale.ROOT).endsWith("." + cleanExtension.toLowerCase(Locale.ROOT));
        |    }
        |
        |    private static void coreDeleteRecursive(File file) {
        |        if (file == null || !file.exists()) {
        |            return;
        |        }
        |
        |        if (file.isDirectory()) {
        |            File[] children = file.listFiles();
        |            if (children != null) {
        |                for (int i = 0; i < children.length; ++i) {
        |                    coreDeleteRecursive(children[i]);
        |                }
        |            }
        |        }
        |
        |        file.delete();
        |    }
        |
        |    private static boolean coreWriteFile(File file, byte[] data) {
        |        FileOutputStream stream = null;
        |        try {
        |            stream = new FileOutputStream(file);
        |            if (data != null && data.length > 0) {
        |                stream.write(data);
        |            }
        |            return true;
        |        } catch (Exception ignored) {
        |            return false;
        |        } finally {
        |            if (stream != null) {
        |                try {
        |                    stream.close();
        |                } catch (Exception ignored) {
        |                }
        |            }
        |        }
        |    }
        |
        |    private static String coreClipboardToken() {
        |        byte[] bytes = new byte[16];
        |        new SecureRandom().nextBytes(bytes);
        |        char[] result = new char[32];
        |        final char[] hex = "0123456789abcdef".toCharArray();
        |        for (int i = 0; i < bytes.length; ++i) {
        |            int value = bytes[i] & 0xff;
        |            result[i * 2] = hex[value >> 4];
        |            result[i * 2 + 1] = hex[value & 15];
        |        }
        |        return new String(result);
        |    }
        |
        |    private static int coreTextInputAction(int editorAction) {
        |        switch (editorAction) {
        |            case EditorInfo.IME_ACTION_DONE:
        |                return CORE_TEXT_INPUT_ACTION_DONE;
        |            case EditorInfo.IME_ACTION_GO:
        |                return CORE_TEXT_INPUT_ACTION_GO;
        |            case EditorInfo.IME_ACTION_SEARCH:
        |                return CORE_TEXT_INPUT_ACTION_SEARCH;
        |            case EditorInfo.IME_ACTION_SEND:
        |                return CORE_TEXT_INPUT_ACTION_SEND;
        |            case EditorInfo.IME_ACTION_NEXT:
        |                return CORE_TEXT_INPUT_ACTION_NEXT;
        |            case EditorInfo.IME_ACTION_PREVIOUS:
        |                return CORE_TEXT_INPUT_ACTION_PREVIOUS;
        |            default:
        |                return CORE_TEXT_INPUT_ACTION_NONE;
        |        }
        |    }
        |
        |    private static byte[] coreUtf8(CharSequence text) {
        |        return text != null ? text.toString().getBytes(StandardCharsets.UTF_8) : new byte[0];
        |    }
        |
        |    private static int coreClamp(int value, int min, int max) {
        |        return value < min ? min : (value > max ? max : value);
        |    }
        |
        |    private static int coreUtf8Offset(String text, int utf16Index) {
        |        if (text == null || text.length() == 0) {
        |            return 0;
        |        }
        |
        |        int index = coreClamp(utf16Index, 0, text.length());
        |        return text.substring(0, index).getBytes(StandardCharsets.UTF_8).length;
        |    }
        |
        |    private static int coreUtf16IndexFromUtf8Offset(String text, int utf8Offset) {
        |        if (text == null || text.length() == 0 || utf8Offset <= 0) {
        |            return 0;
        }
        |
        |        int bytes = 0;
        |        int index = 0;
        |        while (index < text.length()) {
        |            int codePoint = text.codePointAt(index);
        |            int next = index + Character.charCount(codePoint);
        |            int byteCount = text.substring(index, next).getBytes(StandardCharsets.UTF_8).length;
        |            if (bytes + byteCount > utf8Offset) {
        |                break;
        |            }
        |            bytes += byteCount;
        |            index = next;
        |        }
        |        return index;
        |    }
        |
        |    private void coreTextInputEnsureView() {
        |        if (coreTextInputView != null) {
        |            return;
        |        }
        |
        |        coreTextInputView = new CoreTextInputView(this);
        |        addContentView(coreTextInputView, new ViewGroup.LayoutParams(1, 1));
        |    }
        |
        |    private static final class CoreTextInputView extends View {
        |        private final Rect coreTextRect = new Rect();
        |        private String coreText = "";
        |        private int coreSelectionStart;
        |        private int coreSelectionEnd;
        |        private int coreComposingStart = -1;
        |        private int coreComposingEnd = -1;
        |        private int coreInputFlags;
        |        private int coreInputAction;
        |
        |        CoreTextInputView(Context context) {
        |            super(context);
        |            setFocusable(true);
        |            setFocusableInTouchMode(true);
        |        }
        |
        |        private boolean coreHasFlag(int flag) {
        |            return (coreInputFlags & flag) != 0;
        |        }
        |
        |        private int coreInputType() {
        |            int inputType;
        |            if (coreHasFlag(CORE_TEXT_INPUT_FLAG_NUMBER)) {
        |                inputType = InputType.TYPE_CLASS_NUMBER;
        |                if (coreHasFlag(CORE_TEXT_INPUT_FLAG_DECIMAL)) {
        |                    inputType |= InputType.TYPE_NUMBER_FLAG_DECIMAL;
        |                }
        |                if (coreHasFlag(CORE_TEXT_INPUT_FLAG_SIGNED)) {
        |                    inputType |= InputType.TYPE_NUMBER_FLAG_SIGNED;
        |                }
        |                if (coreHasFlag(CORE_TEXT_INPUT_FLAG_PASSWORD)) {
        |                    inputType |= InputType.TYPE_NUMBER_VARIATION_PASSWORD;
        |                }
        |                return inputType;
        |            }
        |
        |            inputType = InputType.TYPE_CLASS_TEXT;
        |            if (coreHasFlag(CORE_TEXT_INPUT_FLAG_EMAIL)) {
        |                inputType |= InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS;
        |            } else if (coreHasFlag(CORE_TEXT_INPUT_FLAG_URI)) {
        |                inputType |= InputType.TYPE_TEXT_VARIATION_URI;
        |            } else if (coreHasFlag(CORE_TEXT_INPUT_FLAG_PASSWORD)) {
        |                inputType |= InputType.TYPE_TEXT_VARIATION_PASSWORD | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
        |            }
        |            if (coreHasFlag(CORE_TEXT_INPUT_FLAG_MULTILINE)) {
        |                inputType |= InputType.TYPE_TEXT_FLAG_MULTI_LINE;
        |            }
        |            if (coreHasFlag(CORE_TEXT_INPUT_FLAG_NO_SUGGESTIONS)) {
        |                inputType |= InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
        |            }
        |            return inputType;
        |        }
        |
        |        private int coreImeOptions() {
        |            int action = EditorInfo.IME_ACTION_NONE;
        |            switch (coreInputAction) {
        |                case CORE_TEXT_INPUT_ACTION_DONE:
        |                    action = EditorInfo.IME_ACTION_DONE;
        |                    break;
        |                case CORE_TEXT_INPUT_ACTION_GO:
        |                    action = EditorInfo.IME_ACTION_GO;
        |                    break;
        |                case CORE_TEXT_INPUT_ACTION_SEARCH:
        |                    action = EditorInfo.IME_ACTION_SEARCH;
        |                    break;
        |                case CORE_TEXT_INPUT_ACTION_SEND:
        |                    action = EditorInfo.IME_ACTION_SEND;
        |                    break;
        |                case CORE_TEXT_INPUT_ACTION_NEXT:
        |                    action = EditorInfo.IME_ACTION_NEXT;
        |                    break;
        |                case CORE_TEXT_INPUT_ACTION_PREVIOUS:
        |                    action = EditorInfo.IME_ACTION_PREVIOUS;
        |                    break;
        |            }
        |            return EditorInfo.IME_FLAG_NO_EXTRACT_UI | action;
        |        }
        |
        |        private void coreSetTextRect(int x, int y, int width, int height) {
        |            int viewHeight = getRootView() != null ? getRootView().getHeight() : getHeight();
        |            int top = viewHeight > 0 ? viewHeight - y - height : y;
        |            coreTextRect.set(x, top, x + width, top + height);
        |            requestRectangleOnScreen(coreTextRect);
        |        }
        |
        |        void coreSetState(int x, int y, int width, int height, int flags, int action, byte[] text, int selectionStart, int selectionEnd, int composingStart, int composingEnd) {
        |            coreInputFlags = flags;
        |            coreInputAction = action;
        |            coreText = text != null ? new String(text, StandardCharsets.UTF_8) : "";
        |            coreSelectionStart = coreUtf16IndexFromUtf8Offset(coreText, selectionStart);
        |            coreSelectionEnd = coreUtf16IndexFromUtf8Offset(coreText, selectionEnd);
        |            if (composingStart != composingEnd) {
        |                coreComposingStart = coreUtf16IndexFromUtf8Offset(coreText, composingStart);
        |                coreComposingEnd = coreUtf16IndexFromUtf8Offset(coreText, composingEnd);
        |            } else {
        |                coreComposingStart = -1;
        |                coreComposingEnd = -1;
        |            }
        |            coreSetTextRect(x, y, width, height);
        |        }
        |
        |        private int coreSelectionMin() {
        |            return Math.min(coreSelectionStart, coreSelectionEnd);
        |        }
        |
        |        private int coreSelectionMax() {
        |            return Math.max(coreSelectionStart, coreSelectionEnd);
        |        }
        |
        |        private boolean coreHasComposingRegion() {
        |            return coreComposingStart >= 0 && coreComposingEnd >= coreComposingStart;
        |        }
        |
        |        private void coreSetSelection(int start, int end, boolean notifyNative) {
        |            coreSelectionStart = coreClamp(start, 0, coreText.length());
        |            coreSelectionEnd = coreClamp(end, 0, coreText.length());
        |            if (notifyNative) {
        |                nativeTextInputSelection(coreUtf8Offset(coreText, coreSelectionStart), coreUtf8Offset(coreText, coreSelectionEnd));
        |            }
        |        }
        |
        |        private void coreReplaceText(int start, int end, CharSequence text, boolean composing) {
        |            String replacement = text != null ? text.toString() : "";
        |            int replaceStart = coreClamp(Math.min(start, end), 0, coreText.length());
        |            int replaceEnd = coreClamp(Math.max(start, end), 0, coreText.length());
        |            coreText = coreText.substring(0, replaceStart) + replacement + coreText.substring(replaceEnd);
        |            int cursor = replaceStart + replacement.length();
        |            coreSetSelection(cursor, cursor, false);
        |            if (composing) {
        |                coreComposingStart = replaceStart;
        |                coreComposingEnd = cursor;
        |            } else {
        |                coreComposingStart = -1;
        |                coreComposingEnd = -1;
        |            }
        |        }
        |
        |        private int coreCodePointOffsetBefore(int from, int count) {
        |            int result = coreClamp(from, 0, coreText.length());
        |            for (int i = 0; i < count && result > 0; ++i) {
        |                result = coreText.offsetByCodePoints(result, -1);
        |            }
        |            return result;
        |        }
        |
        |        private int coreCodePointOffsetAfter(int from, int count) {
        |            int result = coreClamp(from, 0, coreText.length());
        |            for (int i = 0; i < count && result < coreText.length(); ++i) {
        |                result = coreText.offsetByCodePoints(result, 1);
        |            }
        |            return result;
        |        }
        |
        |        private boolean coreDeleteSurroundingText(int beforeStart, int afterEnd) {
        |            int selectionStart = coreSelectionMin();
        |            int selectionEnd = coreSelectionMax();
        |            int deleteStart = coreClamp(beforeStart, 0, selectionStart);
        |            int deleteEnd = coreClamp(afterEnd, selectionEnd, coreText.length());
        |            int beforeBytes = coreText.substring(deleteStart, selectionStart).getBytes(StandardCharsets.UTF_8).length;
        |            int afterBytes = coreText.substring(selectionEnd, deleteEnd).getBytes(StandardCharsets.UTF_8).length;
        |            nativeTextInputDelete(beforeBytes, afterBytes);
        |            coreText = coreText.substring(0, deleteStart) + coreText.substring(deleteEnd);
        |            coreSetSelection(deleteStart, deleteStart, false);
        |            coreComposingStart = -1;
        |            coreComposingEnd = -1;
        |            return true;
        |        }
        |
        |        @Override
        |        public boolean onCheckIsTextEditor() {
        |            return true;
        |        }
        |
        |        @Override
        |        public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        |            outAttrs.inputType = coreInputType();
        |            outAttrs.imeOptions = coreImeOptions();
        |            outAttrs.initialSelStart = coreSelectionStart;
        |            outAttrs.initialSelEnd = coreSelectionEnd;
        |
        |            return new BaseInputConnection(this, false) {
        |                @Override
        |                public CharSequence getTextBeforeCursor(int length, int flags) {
        |                    int end = coreSelectionMin();
        |                    int start = coreClamp(end - Math.max(length, 0), 0, end);
        |                    return coreText.substring(start, end);
        |                }
        |
        |                @Override
        |                public CharSequence getTextAfterCursor(int length, int flags) {
        |                    int start = coreSelectionMax();
        |                    int end = coreClamp(start + Math.max(length, 0), start, coreText.length());
        |                    return coreText.substring(start, end);
        |                }
        |
        |                @Override
        |                public CharSequence getSelectedText(int flags) {
        |                    if (coreSelectionStart == coreSelectionEnd) {
        |                        return "";
        |                    }
        |                    return coreText.substring(coreSelectionMin(), coreSelectionMax());
        |                }
        |
        |                @Override
        |                public ExtractedText getExtractedText(ExtractedTextRequest request, int flags) {
        |                    ExtractedText result = new ExtractedText();
        |                    result.text = coreText;
        |                    result.startOffset = 0;
        |                    result.partialStartOffset = -1;
        |                    result.partialEndOffset = -1;
        |                    result.selectionStart = coreSelectionStart;
        |                    result.selectionEnd = coreSelectionEnd;
        |                    result.flags = 0;
        |                    return result;
        |                }
        |
        |                @Override
        |                public boolean setSelection(int start, int end) {
        |                    coreSetSelection(start, end, true);
        |                    return true;
        |                }
        |
        |                @Override
        |                public boolean setComposingRegion(int start, int end) {
        |                    coreComposingStart = coreClamp(Math.min(start, end), 0, coreText.length());
        |                    coreComposingEnd = coreClamp(Math.max(start, end), 0, coreText.length());
        |                    nativeTextInputComposeRegion(coreUtf8Offset(coreText, coreComposingStart), coreUtf8Offset(coreText, coreComposingEnd));
        |                    return true;
        |                }
        |
        |                @Override
        |                public boolean commitText(CharSequence text, int newCursorPosition) {
        |                    nativeTextInputCommit(coreUtf8(text));
        |                    int start = coreHasComposingRegion() ? coreComposingStart : coreSelectionMin();
        |                    int end = coreHasComposingRegion() ? coreComposingEnd : coreSelectionMax();
        |                    coreReplaceText(start, end, text, false);
        |                    nativeTextInputComposeEnd();
        |                    return true;
        |                }
        |
        |                @Override
        |                public boolean setComposingText(CharSequence text, int newCursorPosition) {
        |                    nativeTextInputCompose(coreUtf8(text));
        |                    int start = coreHasComposingRegion() ? coreComposingStart : coreSelectionMin();
        |                    int end = coreHasComposingRegion() ? coreComposingEnd : coreSelectionMax();
        |                    coreReplaceText(start, end, text, true);
        |                    return true;
        |                }
        |
        |                @Override
        |                public boolean finishComposingText() {
        |                    coreComposingStart = -1;
        |                    coreComposingEnd = -1;
        |                    nativeTextInputComposeEnd();
        |                    return true;
        |                }
        |
        |                @Override
        |                public boolean deleteSurroundingText(int beforeLength, int afterLength) {
        |                    int start = coreClamp(coreSelectionMin() - Math.max(beforeLength, 0), 0, coreSelectionMin());
        |                    int end = coreClamp(coreSelectionMax() + Math.max(afterLength, 0), coreSelectionMax(), coreText.length());
        |                    return coreDeleteSurroundingText(start, end);
        |                }
        |
        |                @Override
        |                public boolean deleteSurroundingTextInCodePoints(int beforeLength, int afterLength) {
        |                    int start = coreCodePointOffsetBefore(coreSelectionMin(), Math.max(beforeLength, 0));
        |                    int end = coreCodePointOffsetAfter(coreSelectionMax(), Math.max(afterLength, 0));
        |                    return coreDeleteSurroundingText(start, end);
        |                }
        |
        |                @Override
        |                public boolean sendKeyEvent(KeyEvent event) {
        |                    if (event == null || event.getAction() != KeyEvent.ACTION_DOWN) {
        |                        return true;
        |                    }
        |
        |                    if (event.getKeyCode() == KeyEvent.KEYCODE_DEL) {
        |                        return deleteSurroundingTextInCodePoints(1, 0);
        |                    }
        |
        |                    if (event.getKeyCode() == KeyEvent.KEYCODE_ENTER) {
        |                        if (coreHasFlag(CORE_TEXT_INPUT_FLAG_MULTILINE)) {
        |                            return commitText("\n", 1);
        |                        }
        |                        nativeTextInputAction(coreInputAction != CORE_TEXT_INPUT_ACTION_NONE ? coreInputAction : CORE_TEXT_INPUT_ACTION_DONE);
        |                        return true;
        |                    }
        |
        |                    int unicode = event.getUnicodeChar();
        |                    if (unicode != 0) {
        |                        return commitText(new String(Character.toChars(unicode)), 1);
        |                    }
        |                    return true;
        |                }
        |
        |                @Override
        |                public boolean performEditorAction(int editorAction) {
        |                    nativeTextInputAction(coreTextInputAction(editorAction));
        |                    return true;
        |                }
        |            };
        |        }
        |    }
        |
        |    private void coreWindowPresentationCapture() {
        |        if (coreWindowPresentationCaptured) {
        |            return;
        |        }
        |
        |        Window window = getWindow();
        |        View decor = window != null ? window.getDecorView() : null;
        |        coreWindowSystemUiVisibility = decor != null ? decor.getSystemUiVisibility() : 0;
        |        coreWindowRequestedOrientation = getRequestedOrientation();
        |        if (window != null) {
        |            coreWindowStatusBarColor = window.getStatusBarColor();
        |            coreWindowNavigationBarColor = window.getNavigationBarColor();
        |            if (Build.VERSION.SDK_INT >= 28) {
        |                WindowManager.LayoutParams attributes = window.getAttributes();
        |                coreWindowCutoutMode = attributes.layoutInDisplayCutoutMode;
        |            }
        |        }
        |        coreWindowPresentationCaptured = true;
        |    }
        |
        |    private static int coreRequestedOrientation(int orientationPolicy) {
        |        switch (orientationPolicy) {
        |            case CORE_WINDOW_ORIENTATION_POLICY_PORTRAIT:
        |                return ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
        |            case CORE_WINDOW_ORIENTATION_POLICY_LANDSCAPE:
        |                return ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
        |            case CORE_WINDOW_ORIENTATION_POLICY_SENSOR:
        |                return ActivityInfo.SCREEN_ORIENTATION_FULL_SENSOR;
        |            case CORE_WINDOW_ORIENTATION_POLICY_SENSOR_PORTRAIT:
        |                return ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;
        |            case CORE_WINDOW_ORIENTATION_POLICY_SENSOR_LANDSCAPE:
        |                return ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
        |            case CORE_WINDOW_ORIENTATION_POLICY_SYSTEM:
        |            default:
        |                return ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
        |        }
        |    }
        |
        |    private void coreWindowPresentationApply() {
        |        coreWindowPresentationCapture();
        |
        |        Window window = getWindow();
        |        if (window == null) {
        |            return;
        |        }
        |
        |        boolean fullscreen = (coreWindowPresentationFlags & CORE_WINDOW_PRESENTATION_FLAG_FULLSCREEN) != 0;
        |        boolean immersive = (coreWindowPresentationFlags & CORE_WINDOW_PRESENTATION_FLAG_IMMERSIVE) != 0;
        |        boolean keepScreenOn = (coreWindowPresentationFlags & CORE_WINDOW_PRESENTATION_FLAG_KEEP_SCREEN_ON) != 0;
        |        boolean edgeToEdge = (coreWindowPresentationFlags & CORE_WINDOW_PRESENTATION_FLAG_EDGE_TO_EDGE) != 0;
        |
        |        if (keepScreenOn) {
        |            window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        |        } else {
        |            window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        |        }
        |
        |        if (fullscreen || immersive) {
        |            window.addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        |        } else {
        |            window.clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        |        }
        |
        |        View decor = window.getDecorView();
        |        if (decor != null) {
        |            int managedFlags = View.SYSTEM_UI_FLAG_FULLSCREEN |
        |                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
        |                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
        |                View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
        |                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
        |                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;
        |            int visibility = coreWindowSystemUiVisibility & ~managedFlags;
        |            if (fullscreen || immersive || edgeToEdge) {
        |                visibility |= View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN;
        |            }
        |            if (immersive || edgeToEdge) {
        |                visibility |= View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;
        |            }
        |            if (fullscreen || immersive) {
        |                visibility |= View.SYSTEM_UI_FLAG_FULLSCREEN;
        |            }
        |            if (immersive) {
        |                visibility |= View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
        |            }
        |            decor.setSystemUiVisibility(visibility);
        |        }
        |
        |        if (edgeToEdge) {
        |            window.setStatusBarColor(Color.TRANSPARENT);
        |            window.setNavigationBarColor(Color.TRANSPARENT);
        |        } else {
        |            window.setStatusBarColor(coreWindowStatusBarColor);
        |            window.setNavigationBarColor(coreWindowNavigationBarColor);
        |        }
        |
        |        if (Build.VERSION.SDK_INT >= 28) {
        |            WindowManager.LayoutParams attributes = window.getAttributes();
        |            attributes.layoutInDisplayCutoutMode = (fullscreen || immersive || edgeToEdge) ? WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES : coreWindowCutoutMode;
        |            window.setAttributes(attributes);
        |        }
        |
        |        int requestedOrientation = coreRequestedOrientation(coreWindowOrientationPolicy);
        |        setRequestedOrientation(requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED ? coreWindowRequestedOrientation : requestedOrientation);
        |    }
        |
        |    public void coreWindowPresentationSet(final int flags, final int orientationPolicy) {
        |        runOnUiThread(new Runnable() {
        |            @Override
        |            public void run() {
        |                coreWindowPresentationFlags = flags;
        |                coreWindowOrientationPolicy = orientationPolicy;
        |                coreWindowPresentationApply();
        |            }
        |        });
        |    }
        |
        |    public String coreAppDataDirectory() {
        |        File directory = getFilesDir();
        |        return directory != null ? directory.getAbsolutePath() : "";
        |    }
        |
        |    public String coreCacheDirectory() {
        |        File directory = getCacheDir();
        |        return directory != null ? directory.getAbsolutePath() : "";
        |    }
        |
        |    public void coreOpenFileDialog(final long requestId, final String extension) {
        |        runOnUiThread(new Runnable() {
        |            @Override
        |            public void run() {
        |                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        |                intent.addCategory(Intent.CATEGORY_OPENABLE);
        |                intent.setType(coreMimeType(extension));
        |                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        |
        |                coreFileDialogRequest = requestId;
        |                try {
        |                    startActivityForResult(intent, CORE_REQUEST_OPEN_DOCUMENT);
        |                } catch (Exception ignored) {
        |                    coreFileDialogRequest = 0;
        |                    nativeFileDialogResult(requestId, false, null);
        |                }
        |            }
        |        });
        |    }
        |
        |    public void coreSaveFileDialog(final long requestId, final String extension) {
        |        runOnUiThread(new Runnable() {
        |            @Override
        |            public void run() {
        |                Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        |                intent.addCategory(Intent.CATEGORY_OPENABLE);
        |                intent.setType(coreMimeType(extension));
        |                intent.putExtra(Intent.EXTRA_TITLE, coreDefaultName(extension));
        |                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        |
        |                coreFileDialogRequest = requestId;
        |                try {
        |                    startActivityForResult(intent, CORE_REQUEST_CREATE_DOCUMENT);
        |                } catch (Exception ignored) {
        |                    coreFileDialogRequest = 0;
        |                    nativeFileDialogResult(requestId, false, null);
        |                }
        |            }
        |        });
        |    }
        |
        |    public void coreOpenDirectoryDialog(final long requestId) {
        |        runOnUiThread(new Runnable() {
        |            @Override
        |            public void run() {
        |                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        |                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
        |
        |                coreFileDialogRequest = requestId;
        |                try {
        |                    startActivityForResult(intent, CORE_REQUEST_OPEN_TREE);
        |                } catch (Exception ignored) {
        |                    coreFileDialogRequest = 0;
        |                    nativeFileDialogResult(requestId, false, null);
        |                }
        |            }
        |        });
        |    }
        |
        |    public int coreOpenContentFd(String uriString, String mode) {
        |        try {
        |            ParcelFileDescriptor descriptor = getContentResolver().openFileDescriptor(Uri.parse(uriString), mode);
        |            if (descriptor == null) {
        |                return -1;
        |            }
        |            return descriptor.detachFd();
        |        } catch (Exception ignored) {
        |            return -1;
        |        }
        |    }
        |
        |    public long coreContentSize(String uriString) {
        |        Uri uri = Uri.parse(uriString);
        |        Cursor cursor = null;
        |        try {
        |            cursor = getContentResolver().query(uri, new String[] { OpenableColumns.SIZE }, null, null, null);
        |            if (cursor != null && cursor.moveToFirst() && !cursor.isNull(0)) {
        |                long size = cursor.getLong(0);
        |                if (size >= 0) {
        |                    return size;
        |                }
        |            }
        |        } catch (Exception ignored) {
        |        } finally {
        |            if (cursor != null) {
        |                cursor.close();
        |            }
        |        }
        |
        |        ParcelFileDescriptor descriptor = null;
        |        try {
        |            descriptor = getContentResolver().openFileDescriptor(uri, "r");
        |            if (descriptor == null) {
        |                return -1;
        |            }
        |            return descriptor.getStatSize();
        |        } catch (Exception ignored) {
        |            return -1;
        |        } finally {
        |            if (descriptor != null) {
        |                try {
        |                    descriptor.close();
        |                } catch (Exception ignored) {
        |                }
        |            }
        |        }
        |    }
        |
        |    public boolean coreContentDelete(String uriString) {
        |        try {
        |            return DocumentsContract.deleteDocument(getContentResolver(), Uri.parse(uriString));
        |        } catch (Exception ignored) {
        |            return false;
        |        }
        |    }
        |
        |    public boolean coreContentIsDirectory(String uriString) {
        |        try {
        |            String type = getContentResolver().getType(Uri.parse(uriString));
        |            return DocumentsContract.Document.MIME_TYPE_DIR.equals(type);
        |        } catch (Exception ignored) {
        |            return false;
        |        }
        |    }
        |
        |    public String coreContentDisplayName(String uriString) {
        |        Uri uri = Uri.parse(uriString);
        |        Cursor cursor = null;
        |        try {
        |            cursor = getContentResolver().query(uri, new String[] { OpenableColumns.DISPLAY_NAME }, null, null, null);
        |            if (cursor != null && cursor.moveToFirst() && !cursor.isNull(0)) {
        |                String name = cursor.getString(0);
        |                if (name != null) {
        |                    return name;
        |                }
        |            }
        |        } catch (Exception ignored) {
        |        } finally {
        |            if (cursor != null) {
        |                cursor.close();
        |            }
        |        }
        |
        |        String segment = uri.getLastPathSegment();
        |        return segment != null ? segment : "";
        |    }
        |
        |    private static Uri coreDocumentUri(Uri uri) {
        |        try {
        |            return DocumentsContract.buildDocumentUriUsingTree(uri, DocumentsContract.getDocumentId(uri));
        |        } catch (Exception ignored) {
        |            return DocumentsContract.buildDocumentUriUsingTree(uri, DocumentsContract.getTreeDocumentId(uri));
        |        }
        |    }
        |
        |    private static Uri coreChildrenUri(Uri uri) {
        |        String documentId;
        |        try {
        |            documentId = DocumentsContract.getDocumentId(uri);
        |        } catch (Exception ignored) {
        |            documentId = DocumentsContract.getTreeDocumentId(uri);
        |        }
        |        return DocumentsContract.buildChildDocumentsUriUsingTree(uri, documentId);
        |    }
        |
        |    private Uri coreParentDocumentUri(Uri uri) {
        |        try {
        |            DocumentsContract.Path path = DocumentsContract.findDocumentPath(getContentResolver(), uri);
        |            List<String> ids = path != null ? path.getPath() : null;
        |            if (ids == null || ids.size() < 2) {
        |                return null;
        |            }
        |            return DocumentsContract.buildDocumentUriUsingTree(uri, ids.get(ids.size() - 2));
        |        } catch (Exception ignored) {
        |            return null;
        |        }
        |    }
        |
        |    private void coreContentListFilesAppend(StringBuilder result, Uri directoryUri, String extension, boolean recursive) {
        |        Cursor cursor = null;
        |        try {
        |            cursor = getContentResolver().query(coreChildrenUri(directoryUri), new String[] {
        |                DocumentsContract.Document.COLUMN_DOCUMENT_ID,
        |                DocumentsContract.Document.COLUMN_DISPLAY_NAME,
        |                DocumentsContract.Document.COLUMN_MIME_TYPE
        |            }, null, null, null);
        |            if (cursor == null) {
        |                return;
        |            }
        |
        |            while (cursor.moveToNext()) {
        |                if (cursor.isNull(0) || cursor.isNull(1)) {
        |                    continue;
        |                }
        |
        |                String displayName = cursor.getString(1);
        |                String mimeType = cursor.isNull(2) ? "" : cursor.getString(2);
        |                Uri childUri = DocumentsContract.buildDocumentUriUsingTree(directoryUri, cursor.getString(0));
        |                if (DocumentsContract.Document.MIME_TYPE_DIR.equals(mimeType)) {
        |                    if (recursive) {
        |                        coreContentListFilesAppend(result, childUri, extension, true);
        |                    }
        |                    continue;
        |                }
        |
        |                if (!coreExtensionMatches(displayName, extension)) {
        |                    continue;
        |                }
        |
        |                if (result.length() > 0) {
        |                    result.append('\n');
        |                }
        |                result.append(childUri.toString());
        |            }
        |        } catch (Exception ignored) {
        |        } finally {
        |            if (cursor != null) {
        |                cursor.close();
        |            }
        |        }
        |    }
        |
        |    private String coreContentListFilesImpl(String uriString, String extension, boolean recursive) {
        |        try {
        |            StringBuilder result = new StringBuilder();
        |            coreContentListFilesAppend(result, Uri.parse(uriString), extension, recursive);
        |            return result.toString();
        |        } catch (Exception ignored) {
        |            return "";
        |        }
        |    }
        |
        |    public String coreContentListFiles(String uriString, String extension) {
        |        return coreContentListFilesImpl(uriString, extension, false);
        |    }
        |
        |    public String coreContentListFilesRecursive(String uriString, String extension) {
        |        return coreContentListFilesImpl(uriString, extension, true);
        |    }
        |
        |    public String coreContentCreateFile(String directoryUriString, String name) {
        |        try {
        |            Uri uri = DocumentsContract.createDocument(getContentResolver(), coreDocumentUri(Uri.parse(directoryUriString)), coreMimeTypeFromName(name), name);
        |            return uri != null ? uri.toString() : "";
        |        } catch (Exception ignored) {
        |            return "";
        |        }
        |    }
        |
        |    public String coreContentCreateDirectory(String directoryUriString, String name) {
        |        try {
        |            Uri uri = DocumentsContract.createDocument(getContentResolver(), coreDocumentUri(Uri.parse(directoryUriString)), DocumentsContract.Document.MIME_TYPE_DIR, name);
        |            return uri != null ? uri.toString() : "";
        |        } catch (Exception ignored) {
        |            return "";
        |        }
        |    }
        |
        |    public String coreContentRename(String uriString, String name) {
        |        try {
        |            Uri uri = DocumentsContract.renameDocument(getContentResolver(), Uri.parse(uriString), name);
        |            return uri != null ? uri.toString() : "";
        |        } catch (Exception ignored) {
        |            return "";
        |        }
        |    }
        |
        |    public String coreContentMove(String uriString, String directoryUriString) {
        |        try {
        |            Uri uri = Uri.parse(uriString);
        |            Uri parentUri = coreParentDocumentUri(uri);
        |            if (parentUri == null) {
        |                return "";
        |            }
        |
        |            Uri result = DocumentsContract.moveDocument(getContentResolver(), uri, parentUri, coreDocumentUri(Uri.parse(directoryUriString)));
        |            return result != null ? result.toString() : "";
        |        } catch (Exception ignored) {
        |            return "";
        |        }
        |    }
        |
        |    public String coreClipboardQueryMediaTypes() {
        |        try {
        |            ClipboardManager clipboard = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        |            if (clipboard == null || !clipboard.hasPrimaryClip()) {
        |                return "";
        |            }
        |
        |            ClipData clip = clipboard.getPrimaryClip();
        |            if (clip == null) {
        |                return "";
        |            }
        |
        |            StringBuilder result = new StringBuilder();
        |            ClipDescription description = clip.getDescription();
        |            if (description != null) {
        |                for (int i = 0; i < description.getMimeTypeCount(); ++i) {
        |                    String mediaType = description.getMimeType(i);
        |                    if (mediaType == null) {
        |                        continue;
        |                    }
        |
        |                    if (mediaType.equals(ClipDescription.MIMETYPE_TEXT_PLAIN) || mediaType.startsWith("text/")) {
        |                        mediaType = "text/plain;charset=utf-8";
        |                    }
        |
        |                    if (result.length() > 0) {
        |                        result.append('\n');
        |                    }
        |                    result.append(mediaType);
        |                }
        |            }
        |            return result.toString();
        |        } catch (Exception ignored) {
        |            return "";
        |        }
        |    }
        |
        |    public byte[] coreClipboardReadItem(String mediaType) {
        |        try {
        |            ClipboardManager clipboard = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        |            if (clipboard == null || !clipboard.hasPrimaryClip()) {
        |                return null;
        |            }
        |
        |            ClipData clip = clipboard.getPrimaryClip();
        |            if (clip == null || clip.getItemCount() == 0) {
        |                return null;
        |            }
        |
        |            boolean wantsText = mediaType != null && (mediaType.equals("text/plain;charset=utf-8") || mediaType.equals(ClipDescription.MIMETYPE_TEXT_PLAIN));
        |            if (wantsText) {
        |                CharSequence text = clip.getItemAt(0).coerceToText(this);
        |                return text != null ? text.toString().getBytes(StandardCharsets.UTF_8) : new byte[0];
        |            }
        |
        |            for (int i = 0; i < clip.getItemCount(); ++i) {
        |                Uri uri = clip.getItemAt(i).getUri();
        |                if (uri == null) {
        |                    continue;
        |                }
        |
        |                String uriType = getContentResolver().getType(uri);
        |                if (mediaType != null && uriType != null && !mediaType.equals(uriType)) {
        |                    continue;
        |                }
        |
        |                InputStream stream = null;
        |                try {
        |                    stream = getContentResolver().openInputStream(uri);
        |                    if (stream == null) {
        |                        continue;
        |                    }
        |
        |                    ByteArrayOutputStream output = new ByteArrayOutputStream();
        |                    byte[] buffer = new byte[8192];
        |                    while (true) {
        |                        int count = stream.read(buffer);
        |                        if (count < 0) {
        |                            break;
        |                        }
        |                        output.write(buffer, 0, count);
        |                    }
        |                    return output.toByteArray();
        |                } finally {
        |                    if (stream != null) {
        |                        stream.close();
        |                    }
        |                }
        |            }
        |        } catch (Exception ignored) {
        |        }
        |
        |        return null;
        |    }
        |
        |    public boolean coreClipboardWriteItems(String[] mediaTypes, byte[][] dataItems) {
        |        try {
        |            if (mediaTypes == null || dataItems == null || mediaTypes.length == 0 || mediaTypes.length != dataItems.length) {
        |                return false;
        |            }
        |
        |            ClipboardManager clipboard = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        |            if (clipboard == null) {
        |                return false;
        |            }
        |
        |            if (mediaTypes.length == 1 && (mediaTypes[0].equals("text/plain;charset=utf-8") || mediaTypes[0].equals(ClipDescription.MIMETYPE_TEXT_PLAIN))) {
        |                String text = dataItems[0] != null ? new String(dataItems[0], StandardCharsets.UTF_8) : "";
        |                clipboard.setPrimaryClip(ClipData.newPlainText("Core", text != null ? text : ""));
        |                return true;
        |            }
        |
        |            File root = new File(getCacheDir(), "core_clipboard");
        |            coreDeleteRecursive(root);
        |            if (!root.mkdirs() && !root.isDirectory()) {
        |                return false;
        |            }
        |
        |            String token = coreClipboardToken();
        |            File tokenDirectory = new File(root, token);
        |            if (!tokenDirectory.mkdirs() && !tokenDirectory.isDirectory()) {
        |                return false;
        |            }
        |
        |            String[] descriptionTypes = new String[mediaTypes.length];
        |            for (int i = 0; i < mediaTypes.length; ++i) {
        |                if (mediaTypes[i] == null || mediaTypes[i].length() == 0) {
        |                    return false;
        |                }
        |
        |                descriptionTypes[i] = mediaTypes[i].equals("text/plain;charset=utf-8") ? ClipDescription.MIMETYPE_TEXT_PLAIN : mediaTypes[i];
        |                if (!coreWriteFile(new File(tokenDirectory, i + ".type"), mediaTypes[i].getBytes(StandardCharsets.UTF_8))) {
        |                    return false;
        |                }
        |                if (!coreWriteFile(new File(tokenDirectory, i + ".bin"), dataItems[i])) {
        |                    return false;
        |                }
        |            }
        |
        |            ClipDescription description = new ClipDescription("Core", descriptionTypes);
        |            ClipData clip = new ClipData(description, new ClipData.Item(CoreClipboardProvider.uriFor(getPackageName(), token, 0)));
        |            for (int i = 1; i < mediaTypes.length; ++i) {
        |                clip.addItem(new ClipData.Item(CoreClipboardProvider.uriFor(getPackageName(), token, i)));
        |            }
        |            clipboard.setPrimaryClip(clip);
        |            return true;
        |        } catch (Exception ignored) {
        |            return false;
        |        }
        |    }
        |
        |    @Override
        |    protected void onDestroy() {
        |        coreLoadNativeLibrary();
        |        nativeActivityDestroying(isChangingConfigurations(), isFinishing());
        |        super.onDestroy();
        |    }
        |
        |    @Override
        |    public void onTrimMemory(int level) {
        |        super.onTrimMemory(level);
        |        if (level >= ComponentCallbacks2.TRIM_MEMORY_RUNNING_LOW && level != ComponentCallbacks2.TRIM_MEMORY_UI_HIDDEN) {
        |            coreLoadNativeLibrary();
        |            nativeMemoryPressure();
        |        }
        |    }
        |
        |    @Override
        |    protected void onCreate(Bundle savedInstanceState) {
        |        coreLoadNativeLibrary();
        |        super.onCreate(savedInstanceState);
        |        coreTextInputEnsureView();
        |        coreWindowPresentationCapture();
        |        coreWindowPresentationApply();
        |    }
        |
        |    @Override
        |    protected void onResume() {
        |        super.onResume();
        |        coreWindowPresentationApply();
        |    }
        |
        |    @Override
        |    public void onWindowFocusChanged(boolean hasFocus) {
        |        super.onWindowFocusChanged(hasFocus);
        |        if (hasFocus) {
        |            coreWindowPresentationApply();
        |        }
        |    }
        |
        |    public void coreTextInputSet(final boolean enabled, final int x, final int y, final int width, final int height, final int flags, final int action, final byte[] text, final int selectionStart, final int selectionEnd, final int composingStart, final int composingEnd) {
        |        runOnUiThread(new Runnable() {
        |            @Override
        |            public void run() {
        |                if (!enabled) {
        |                    if (coreTextInputView == null) {
        |                        return;
        |                    }
        |
        |                    InputMethodManager manager = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
        |                    if (manager != null) {
        |                        manager.hideSoftInputFromWindow(coreTextInputView.getWindowToken(), 0);
        |                    }
        |                    coreTextInputView.clearFocus();
        |                    return;
        |                }
        |
        |                coreTextInputEnsureView();
        |                coreTextInputView.coreSetState(x, y, width, height, flags, action, text, selectionStart, selectionEnd, composingStart, composingEnd);
        |                coreTextInputView.requestFocus();
        |                InputMethodManager manager = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
        |                if (manager != null) {
        |                    manager.restartInput(coreTextInputView);
        |                    manager.showSoftInput(coreTextInputView, InputMethodManager.SHOW_IMPLICIT);
        |                }
        |            }
        |        });
        |    }
        |
        |    @Override
        |    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        |        if (requestCode == CORE_REQUEST_OPEN_DOCUMENT || requestCode == CORE_REQUEST_CREATE_DOCUMENT || requestCode == CORE_REQUEST_OPEN_TREE) {
        |            long requestId = coreFileDialogRequest;
        |            coreFileDialogRequest = 0;
        |
        |            String uriString = null;
        |            if (resultCode == RESULT_OK && data != null && data.getData() != null) {
        |                Uri uri = data.getData();
        |                int flags = data.getFlags() & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
        |                if (flags != 0) {
        |                    try {
        |                        getContentResolver().takePersistableUriPermission(uri, flags);
        |                    } catch (Exception ignored) {
        |                    }
        |                }
        |                uriString = uri.toString();
        |            }
        |
        |            nativeFileDialogResult(requestId, uriString != null, uriString);
        |            return;
        |        }
        |
        |        super.onActivityResult(requestCode, resultCode, data);
        |    }
        |}
        |
    ]=])
    string(REGEX REPLACE "(^|\n)        \\|" "\\1" CORE_ANDROID_JAVA_SOURCE_CONTENT "${CORE_ANDROID_JAVA_SOURCE_CONTENT}")
    file(WRITE "${CORE_ANDROID_JAVA_SOURCE}" "${CORE_ANDROID_JAVA_SOURCE_CONTENT}")

    set(CORE_ANDROID_CLIPBOARD_PROVIDER_JAVA_SOURCE_CONTENT [=[
        |package core.android;
        |
        |import android.content.ContentProvider;
        |import android.content.ContentValues;
        |import android.database.Cursor;
        |import android.database.MatrixCursor;
        |import android.net.Uri;
        |import android.os.ParcelFileDescriptor;
        |import android.provider.OpenableColumns;
        |
        |import java.io.File;
        |import java.io.FileInputStream;
        |import java.io.FileNotFoundException;
        |import java.nio.charset.StandardCharsets;
        |
        |public class CoreClipboardProvider extends ContentProvider {
        |    private static final String ROOT = "core_clipboard";
        |
        |    public static Uri uriFor(String packageName, String token, int index) {
        |        return Uri.parse("content://" + packageName + ".core.clipboard/clipboard/" + token + "/" + index);
        |    }
        |
        |    @Override
        |    public boolean onCreate() {
        |        return true;
        |    }
        |
        |    private File coreFile(Uri uri, String extension) {
        |        if (getContext() == null || uri == null || uri.getPathSegments().size() != 3) {
        |            return null;
        }
        |
        |        String root = uri.getPathSegments().get(0);
        |        String token = uri.getPathSegments().get(1);
        |        String index = uri.getPathSegments().get(2);
        |        if (!root.equals("clipboard") || !token.matches("[0-9a-f]{32}") || !index.matches("[0-9]+")) {
        |            return null;
        }
        |
        |        return new File(new File(new File(getContext().getCacheDir(), ROOT), token), index + extension);
        |    }
        |
        |    @Override
        |    public String getType(Uri uri) {
        |        File typeFile = coreFile(uri, ".type");
        |        if (typeFile == null || !typeFile.isFile()) {
        |            return null;
        }
        |
        |        FileInputStream stream = null;
        |        try {
        |            stream = new FileInputStream(typeFile);
        |            byte[] data = new byte[(int)typeFile.length()];
        |            int offset = 0;
        |            while (offset < data.length) {
        |                int count = stream.read(data, offset, data.length - offset);
        |                if (count < 0) {
        |                    break;
        |                }
        |                offset += count;
        |            }
        |            return new String(data, 0, offset, StandardCharsets.UTF_8);
        |        } catch (Exception ignored) {
        |            return null;
        |        } finally {
        |            if (stream != null) {
        |                try {
        |                    stream.close();
        |                } catch (Exception ignored) {
        |                }
        |            }
        |        }
        |    }
        |
        |    @Override
        |    public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        |        if (mode != null && mode.indexOf('w') >= 0) {
        |            throw new FileNotFoundException();
        |        }
        |
        |        File dataFile = coreFile(uri, ".bin");
        |        if (dataFile == null || !dataFile.isFile()) {
        |            throw new FileNotFoundException();
        }
        |
        |        return ParcelFileDescriptor.open(dataFile, ParcelFileDescriptor.MODE_READ_ONLY);
        |    }
        |
        |    @Override
        |    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
        |        File dataFile = coreFile(uri, ".bin");
        |        if (dataFile == null || !dataFile.isFile()) {
        |            return null;
        }
        |
        |        MatrixCursor cursor = new MatrixCursor(new String[] { OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE });
        |        cursor.addRow(new Object[] { dataFile.getName(), dataFile.length() });
        |        return cursor;
        |    }
        |
        |    @Override
        |    public Uri insert(Uri uri, ContentValues values) {
        |        return null;
        |    }
        |
        |    @Override
        |    public int delete(Uri uri, String selection, String[] selectionArgs) {
        |        return 0;
        |    }
        |
        |    @Override
        |    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        |        return 0;
        |    }
        |}
        |
    ]=])
    string(REGEX REPLACE "(^|\n)        \\|" "\\1" CORE_ANDROID_CLIPBOARD_PROVIDER_JAVA_SOURCE_CONTENT "${CORE_ANDROID_CLIPBOARD_PROVIDER_JAVA_SOURCE_CONTENT}")
    file(WRITE "${CORE_ANDROID_CLIPBOARD_PROVIDER_JAVA_SOURCE}" "${CORE_ANDROID_CLIPBOARD_PROVIDER_JAVA_SOURCE_CONTENT}")

    list(APPEND TARGET_PROPERTIES
        CORE_ANDROID_JAVA_SOURCE "${CORE_ANDROID_JAVA_SOURCE}"
        CORE_ANDROID_CLIPBOARD_PROVIDER_JAVA_SOURCE "${CORE_ANDROID_CLIPBOARD_PROVIDER_JAVA_SOURCE}"
        CORE_ANDROID_JAVA_SOURCE_DIR "${CORE_ANDROID_JAVA_SOURCE_DIR}"
    )
endif()

if(CORE_BUILD_STATIC)
    add_library(core STATIC ${HEADER_FILES} ${SOURCE_FILES})
else()
    add_library(core SHARED ${HEADER_FILES} ${SOURCE_FILES})
endif()

target_link_libraries(core PUBLIC ${LIBS})
set_target_properties(core
    PROPERTIES ${TARGET_PROPERTIES}
)

include(GenerateExportHeader)
generate_export_header(core
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/export.h"
    INCLUDE_GUARD_NAME "CORE_API_H"
    EXPORT_MACRO_NAME "CORE_API"
    NO_EXPORT_MACRO_NAME "CORE_NO_API"
    DEPRECATED_MACRO_NAME "CORE_API_DEPRECATED"
    NO_DEPRECATED_MACRO_NAME "CORE_API_NO_DEPRECATED"
)