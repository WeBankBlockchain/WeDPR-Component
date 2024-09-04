package com.webank.wedpr.sdk.jni.jmh;

import java.util.concurrent.TimeUnit;
import org.openjdk.jmh.results.format.ResultFormatType;
import org.openjdk.jmh.runner.Runner;
import org.openjdk.jmh.runner.options.Options;
import org.openjdk.jmh.runner.options.OptionsBuilder;
import org.openjdk.jmh.runner.options.TimeValue;

public class BenchmarkMain {
    public static void main(String[] args) throws Exception {
        Options opt =
                new OptionsBuilder()
                        .include("com.webank.wedpr.sdk.jni.jmh.*")
                        .forks(1)
                        .timeUnit(TimeUnit.NANOSECONDS)
                        .warmupIterations(5)
                        .warmupTime(TimeValue.valueOf("1s"))
                        .measurementIterations(60)
                        .measurementTime(TimeValue.valueOf("1s"))
                        // .resultFormat(ResultFormatType.JSON)
                        .resultFormat(ResultFormatType.TEXT)
                        .build();

        new Runner(opt).run();
    }
}