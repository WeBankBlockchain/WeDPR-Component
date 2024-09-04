/**
 * Copyright 2023 [wedpr]
 *
 * <p>Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 * <p>http://www.apache.org/licenses/LICENSE-2.0
 *
 * <p>Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.webank.wedpr.sdk.jni.common;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Field;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.util.Objects;
import java.util.Scanner;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Helper class to load ppc-crypto-sdk jni resources.
 *
 * @author octopus
 */
public final class JniLibLoader {
    private static final Logger logger = LoggerFactory.getLogger(JniLibLoader.class);

    public static final String LINUX = "linux";
    public static final String MAC = "mac";
    public static final String WIN = "win";

    public static final String NATIVE_RESOURCE_LIB_NAME = "ppc-crypto-sdk-jni";
    public static final String NATIVE_RESOURCE_HOME = "/META-INF/native";
    public static final String NATIVE_WIN_DEPS_DIR = NATIVE_RESOURCE_HOME + "/win/";
    public static final String WIN_DEPS_FILE_LIST = "file.list";

    public static final String OS_NAME = getOs();
    public static final String ARCH_NAME = getArch();

    private static File WORKDIR;

    static {
        // 1. initialize workdir

        // -Dcom.webank.wedpr.workdir
        String workdir = System.getProperty("com.webank.wedpr.workdir");
        if (workdir != null) {
            try {
                File f = new File(workdir);
                f.mkdirs();
                WORKDIR = f;
                logger.info("initialize workdir, -Dcom.webank.wedpr.workdir: {}", WORKDIR);
            } catch (Exception e) {
                logger.debug("initialize workdir, cannot mkdir workdir: {}, e: ", workdir, e);
                workdir = null;
            }
        }

        // default -Djava.io.tmpdir
        if (workdir == null) {
            workdir = System.getProperty("java.io.tmpdir");
            File f = new File(workdir);
            f.mkdirs();
            WORKDIR = f;

            logger.info("initialize workdir, -Djava.io.tmpdirs: {}", WORKDIR);
        }
    }

    public static String getArch() {
        String archName = System.getProperty("os.arch", "");
        if (archName.contains("aarch64") || archName.contains("arm64")) {
            return "arm";
        } else {
            return "";
        }
    }

    public static String getLibName(String baseName) {
        String osName = OS_NAME;
        String archName = ARCH_NAME;

        logger.debug("osName: {}, archName: {}", osName, archName);

        if (osName.contains(WIN)) {
            return baseName + ".dll";
        } else if (osName.contains(LINUX)) {
            String arch = getArch();
            if ("arm".equals(arch)) {
                return "lib" + baseName + "-aarch64" + ".so";
            }
            return "lib" + baseName + ".so";
        } else if (osName.contains(MAC)) {
            String arch = getArch();
            if ("arm".equals(arch)) {
                return "lib" + baseName + "-aarch64" + ".dylib";
            }
            return "lib" + baseName + ".dylib";
        } else {
            throw new RuntimeException("unrecognized OS: " + osName);
        }
    }

    public static String getOs() {
        String osName = System.getProperty("os.name").toLowerCase();
        if (osName.contains(WIN)) {
            return WIN;
        } else if (osName.contains(LINUX)) {
            return LINUX;
        } else if (osName.contains(MAC)) {
            return MAC;
        } else {
            throw new RuntimeException("unrecognized OS: " + osName);
        }
    }

    public static void loadJniLibrary() {
        String libPath = null;
        boolean loadLibFromFsOk = false;
        try {
            // 1. try to load library from filesystem
            //  -Dcom.webank.wedpr.jni.library.path
            libPath = System.getProperty("com.webank.wedpr.jni.library.path");
            if (libPath != null) {
                logger.info(
                        "try to load library -Dcom.webank.wedpr.jni.library.path, libPath: {}",
                        libPath);
                // try to load library from filesystem first
                loadLibraryFromFs(
                        libPath, NATIVE_RESOURCE_LIB_NAME, new File(libPath).isAbsolute());
                loadLibFromFsOk = true;
            }
        } catch (Exception e0) {
            logger.error("unable to load library from fs, libDir: {}, e: ", libPath, e0);
        }

        // 2. try to load library from jar
        if (!loadLibFromFsOk) {
            try {
                logger.info("try to load library from jar");
                if (Objects.equals(getOs(), WIN)) {
                    loadWinDepsLibraryFromJar();
                }
                loadLibraryFromJar(NATIVE_RESOURCE_LIB_NAME);
            } catch (Exception e1) {
                logger.error("unable to load library from fs, e: ", e1);
                throw new UnsatisfiedLinkError(e1.toString());
            }
        }
    }

    public static void loadLibrary(String lib, boolean absolute) {
        if (absolute) {
            System.load(lib);
        } else {
            System.loadLibrary(lib);
        }
    }

    public static void addLibraryDir(String libraryPath)
            throws NoSuchFieldException, IllegalAccessException {
        Field userPathsField = ClassLoader.class.getDeclaredField("usr_paths");
        userPathsField.setAccessible(true);
        String[] paths = (String[]) userPathsField.get(null);
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < paths.length; i++) {
            if (libraryPath.equals(paths[i])) {
                continue;
            }
            sb.append(paths[i]).append(File.pathSeparator);
        }
        sb.append(libraryPath);
        System.setProperty("java.library.path", sb.toString());
        logger.info("modify, java.library.path: " + sb.toString());
        final Field sysPathsField = ClassLoader.class.getDeclaredField("sys_paths");
        sysPathsField.setAccessible(true);
        sysPathsField.set(null, null);
    }

    public static void loadLibraryFromFs(String dir, String libName, boolean absolute) {
        String resource = dir + "/" + getLibName(libName);
        loadLibrary(resource, absolute);
    }

    public static void loadLibraryFromJar(String baseName)
            throws IOException, NoSuchFieldException, IllegalAccessException {
        String libName = getLibName(baseName);
        File tempDir = new File(WORKDIR, String.valueOf(System.nanoTime()));
        // create temp dir
        tempDir.mkdirs();
        File tempFile = new File(tempDir, libName);
        logger.info("tempDir: {}, tempFile: {}", tempDir, tempFile);

        String resource = NATIVE_RESOURCE_HOME + "/" + libName;
        try (InputStream is = JniLibLoader.class.getResourceAsStream(resource)) {
            Files.copy(is, tempFile.toPath(), StandardCopyOption.REPLACE_EXISTING);
        } catch (IOException e) {
            try {
                tempFile.delete();
            } catch (Exception e0) {
                logger.debug("delete temp file error, e: ", e0);
            }
            logger.error("copy dynamic lib from jar failed, resource: {}, e: ", resource, e);
            throw e;
        } catch (NullPointerException e) {
            throw new FileNotFoundException("Cannot found " + resource + " inside the JAR.");
        }

        try {
            loadLibrary(tempFile.getAbsolutePath(), true);
        } catch (Exception e) {
            logger.error("loadLibrary error, resource: {}, e: ", resource, e);
            throw e;
        } finally {
            try {
                tempFile.delete();
                tempDir.delete();
                logger.debug("remove temp dir and temp file, dir: {}, file: {}", tempDir, tempFile);
            } catch (Exception ignored) {
            }
        }
    }

    public static void loadWinDepsLibraryFromJar() throws IOException, NoSuchFieldException {
        File tempDir = new File(WORKDIR, String.valueOf(System.nanoTime()));
        tempDir.mkdirs();

        try (InputStream is =
                JniLibLoader.class.getResourceAsStream(NATIVE_WIN_DEPS_DIR + WIN_DEPS_FILE_LIST)) {
            if (is == null) {
                throw new FileNotFoundException(
                        "Cannot found " + WIN_DEPS_FILE_LIST + " inside the JAR.");
            }
            try (Scanner scanner = new Scanner(is)) {
                while (scanner.hasNext()) {
                    String lib = scanner.next();
                    File targetFile = new File(tempDir, lib);
                    logger.info(
                            "loadWinDepsLibraryFromJar tempDir: {}, targetFile: {}",
                            tempDir,
                            targetFile);
                    try (InputStream fis =
                            JniLibLoader.class.getResourceAsStream(NATIVE_WIN_DEPS_DIR + lib)) {
                        if (fis == null) {
                            throw new FileNotFoundException(
                                    "Cannot found " + lib + " inside the JAR.");
                        }
                        Files.copy(fis, targetFile.toPath(), StandardCopyOption.REPLACE_EXISTING);
                        loadLibrary(targetFile.getAbsolutePath(), true);
                    } catch (IOException e) {
                        targetFile.delete();
                    } catch (Exception e) {
                        logger.error(
                                "loadLibrary error, resource: {}, e: ",
                                targetFile.getAbsolutePath(),
                                e);
                        throw e;
                    } finally {
                        try {
                            targetFile.delete();
                            tempDir.delete();
                            logger.debug("remove temp dir and temp file, {}", targetFile);
                        } catch (Exception ignored) {
                        }
                    }
                }
            }
        }
    }
}
