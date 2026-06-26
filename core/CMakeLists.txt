set(HEADER_FILES
    base64.h
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
    source_location.h
    tester.h
    containers/array.h
    containers/hash_set.h
    containers/hash_table.h
    containers/ring_buffer.h
    containers/span.h
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
    memory/memory.h
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
    ecs.cpp
    json.cpp
    tester.cpp
    memory/memory.cpp
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
    target_link_libraries(core-platform-macos PRIVATE ${LIBS} "-framework Cocoa" "-framework UniformTypeIdentifiers")

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

    file(MAKE_DIRECTORY "${CORE_ANDROID_JAVA_PACKAGE_DIR}")
    set(CORE_ANDROID_JAVA_SOURCE_CONTENT [=[
        |package core.android;
        |
        |import android.app.NativeActivity;
        |import android.content.ClipData;
        |import android.content.ClipDescription;
        |import android.content.ClipboardManager;
        |import android.content.Context;
        |import android.content.Intent;
        |import android.database.Cursor;
        |import android.net.Uri;
        |import android.os.ParcelFileDescriptor;
        |import android.provider.OpenableColumns;
        |import android.webkit.MimeTypeMap;
        |
        |import java.io.ByteArrayOutputStream;
        |import java.io.InputStream;
        |import java.nio.charset.StandardCharsets;
        |import java.util.Locale;
        |
        |public class CoreNativeActivity extends NativeActivity {
        |    private static final int CORE_REQUEST_OPEN_DOCUMENT = 0x434f5245;
        |    private static final int CORE_REQUEST_CREATE_DOCUMENT = 0x434f5246;
        |
        |    private long coreFileDialogRequest;
        |
        |    private static native void nativeFileDialogResult(long requestId, boolean accepted, String uri);
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
        |    public boolean coreClipboardWriteItem(String mediaType, byte[] data) {
        |        try {
        |            if (mediaType == null || (!mediaType.equals("text/plain;charset=utf-8") && !mediaType.equals(ClipDescription.MIMETYPE_TEXT_PLAIN))) {
        |                return false;
        |            }
        |
        |            ClipboardManager clipboard = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        |            if (clipboard == null) {
        |                return false;
        |            }
        |
        |            String text = data != null ? new String(data, StandardCharsets.UTF_8) : "";
        |            clipboard.setPrimaryClip(ClipData.newPlainText("Core", text != null ? text : ""));
        |            return true;
        |        } catch (Exception ignored) {
        |            return false;
        |        }
        |    }
        |
        |    @Override
        |    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        |        if (requestCode == CORE_REQUEST_OPEN_DOCUMENT || requestCode == CORE_REQUEST_CREATE_DOCUMENT) {
        |            long requestId = coreFileDialogRequest;
        |            coreFileDialogRequest = 0;
        |
        |            String uriString = null;
        |            if (resultCode == RESULT_OK && data != null && data.getData() != null) {
        |                Uri uri = data.getData();
        |                int flags = data.getFlags() & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
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

    list(APPEND TARGET_PROPERTIES
        CORE_ANDROID_JAVA_SOURCE "${CORE_ANDROID_JAVA_SOURCE}"
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