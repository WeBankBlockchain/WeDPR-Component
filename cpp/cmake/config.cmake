hunter_config(tarscpp VERSION 3.0.4-local
        URL https://${URL_BASE}/FISCO-BCOS/TarsCpp/archive/e1a1df43e42a636d26c59f7fc12f62ae756d9ffc.tar.gz
        SHA1 8632a852c0dbf367d8d4c4d2957fde2ff713abe0
        )
hunter_config(cryptopp VERSION 8.6.0
        URL "https://${URL_BASE}/FISCO-BCOS/cryptopp/archive/1887f2f05679debf5d5b9553b8d1a686049eb8c0.tar.gz"
        SHA1 062e044d15415151ddebe67cce63fbaeba4ba596
        )

# Note: mysql-connector-cpp default use the tassl
hunter_config(mysql-connector-cpp VERSION 8.0.31-local
        URL "https://github.com/mysql/mysql-connector-cpp/archive/8.0.31.tar.gz"
        SHA1 082c58faa4fbf61b796ccdb0b075b4cb3c331b9d
        CMAKE_ARGS WITH_HUNTER_OpenSSl=ON WITH_HUNTER_PROTOBUF=ON
        BUILDTYPE_DOCSTRING=${CMAKE_BUILD_TYPE}
        )

hunter_config(OpenSSL VERSION tassl_1.1.1b_v1.4-local
        URL https://${URL_BASE}/FISCO-BCOS/TASSL-1.1.1b/archive/5144cc460da7b7de36e27780aba0fd928da222ed.tar.gz
        SHA1 e0d6de3a5abb2ea09cdf3fd22e50bd8b866125d6
        )

hunter_config(hiredis VERSION ${HUNTER_hiredis_VERSION})
hunter_config(redis-plus-plus VERSION 1.3.6-local
        URL https://${URL_BASE}/sewenew/redis-plus-plus/archive/1.3.6.tar.gz
        SHA1 650a9fc65c958119f5360ae7dc2341487a29ac6d
        CMAKE_ARGS REDIS_PLUS_PLUS_BUILD_STATIC=ON)

hunter_config(SEAL VERSION 4.0.0
        URL https://${URL_BASE}/microsoft/SEAL/archive/4.0.0.tar.gz
        SHA1 1ed568161c784d41a485590b08f841d213726e76
        CMAKE_ARGS SEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=OFF
        )

hunter_config(Kuku VERSION 2.1.0
        URL https://${URL_BASE}/microsoft/Kuku/archive/2.1.0.tar.gz
        SHA1 9dc206b885bd8da17877d78849c1b17a29bef59f
        )
hunter_config(tbb VERSION 2021.3.0 CMAKE_ARGS BUILD_SHARED_LIBS=OFF)


# the dependencies for libhdfs3
hunter_config(Protobuf VERSION "3.19.4-p0"
        URL https://github.com/cpp-pm/protobuf/archive/v3.19.4-p0.tar.gz
        SHA1 e5b797dbc4e6ad92d0924ae86c130be4354c35b6
        CMAKE_ARGS CMAKE_POSITION_INDEPENDENT_CODE=TRUE
        )
hunter_config(libxml2 VERSION "2.9.7-p0" CMAKE_ARGS CMAKE_POSITION_INDEPENDENT_CODE=TRUE)
hunter_config(uuid VERSION 1.0.3 CMAKE_ARGS CMAKE_POSITION_INDEPENDENT_CODE=TRUE)

hunter_config(cpu_features VERSION 0.7.0
        URL https://${URL_BASE}/google/cpu_features/archive/v0.7.0.tar.gz
        SHA1 dacec18454c542ef450c883f45be7b2deaba7a91
        CMAKE_ARGS
        CMAKE_POSITION_INDEPENDENT_CODE=TRUE
        BUILD_TESTING=OFF
        )

hunter_config(libhdfs3 VERSION 3.0.0-local
        URL https://${URL_BASE}/FISCO-BCOS/libhdfs3/archive/f63e08d0c45f76591e448b56eb3f1b4e7de09c13.tar.gz
        SHA1 88fdd418500478ea35fb09911d088430f0835893
        CMAKE_ARGS
        ENABLE_BOOST=OFF
        ENABLE_COVERAGE=OFF
        ENABLE_SSE=${ENABLE_SSE}
        )

hunter_config(ipp-crypto VERSION 202104-local
        URL https://${URL_BASE}/intel/ipp-crypto/archive/ippcp_2021.4.tar.gz
        SHA1 72edc2d16c30ec1612f45793befbc5689f72970f
        CMAKE_ARGS
        WITH_HUNTER_OpenSSl=ON
        )
        