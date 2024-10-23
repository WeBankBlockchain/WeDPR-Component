
# ipp-crypto
#if(ENABLE_IPP_CRYPTO)
#    hunter_add_package(ipp-crypto)
#endif()

######## common dependencies ######## 
include(InstallBcosUtilities)
if (TESTS)
    find_package(Boost COMPONENTS unit_test_framework)
endif()

# cpp_features
if(ENABLE_CPU_FEATURES)
    find_package(CpuFeatures REQUIRED)
endif()
######## common dependencies end ########

##### the wedpr_toolkit #####
if(BUILD_ALL OR BUILD_WEDPR_TOOLKIT)
    find_package(TBB REQUIRED)
    find_package(gRPC REQUIRED)
    find_package(jsoncpp REQUIRED)
endif()
##### the sdk dependencies #####
if(BUILD_ALL OR BUILD_SDK OR BUILD_UDF)
    find_package(OpenSSL REQUIRED)
    find_package(unofficial-sodium CONFIG REQUIRED)
endif()

##### the full-dependencies #####
if(BUILD_ALL)
    find_package(${BCOS_BOOSTSSL_TARGET} REQUIRED)
    # tcmalloc
    include(ProjectTCMalloc)

    find_package(SEAL REQUIRED)
    find_package(Kuku REQUIRED)

    # APSI: Note: APSI depends on seal 4.0 and Kuku 2.1
    include(ProjectAPSI)
    # Wedpr Crypto
    include(ProjectWedprCrypto)
    include(Installlibhdfs3)
endif()
##### the full-dependencies end #####

##### the sdk-dependencies #####
# find JNI
set(JAVA_AWT_LIBRARY NotNeeded)
set(JAVA_JVM_LIBRARY NotNeeded)
find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})
##### the sdk-dependencies end#####