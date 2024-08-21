package com.webank.wedpr.sdk.jni.jmh;

import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.homo.Paillier;
import org.openjdk.jmh.annotations.Benchmark;
import org.openjdk.jmh.annotations.BenchmarkMode;
import org.openjdk.jmh.annotations.Measurement;
import org.openjdk.jmh.annotations.Mode;
import org.openjdk.jmh.annotations.OutputTimeUnit;
import org.openjdk.jmh.annotations.Scope;
import org.openjdk.jmh.annotations.Setup;
import org.openjdk.jmh.annotations.State;
import org.openjdk.jmh.annotations.TearDown;
import org.openjdk.jmh.annotations.Warmup;
import org.openjdk.jmh.runner.Runner;
import org.openjdk.jmh.runner.options.Options;
import org.openjdk.jmh.runner.options.OptionsBuilder;

import java.math.BigInteger;
import java.util.concurrent.TimeUnit;

@State(Scope.Thread)
@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.NANOSECONDS)
@Warmup(iterations = 5, time = 1, timeUnit = TimeUnit.SECONDS)
@Measurement(iterations = 120, time = 1, timeUnit = TimeUnit.SECONDS)
public class PaillierBenchmark
{
    private Paillier paillier = new Paillier();
    // the keypair
    private long keypair;
    // the public key
    private long publickey;
    // m1
    private byte[] m1;
    // m2
    private byte[] m2;
    // v
    private byte[] v;
    int keyBits;
    // cipher1
    public byte[] c1;
    public byte[] c2;

    @Setup
    public void setup() throws JniException {
        m1 = BigInteger.valueOf(System.currentTimeMillis()).toByteArray();
        m2 = BigInteger.valueOf(System.currentTimeMillis() + 100000).toByteArray();
        v = BigInteger.valueOf(2134234).toByteArray();
        keyBits = 2048;
        keypair = paillier.generateKeyPair(2048);
        publickey = paillier.getPublicKeyJniObject(keypair);
        c1 = paillier.encryptFast(m1, keypair);
        c2 = paillier.encryptFast(m2, keypair);
    }

    @TearDown
    public void tearDown() {
        // Clean up resources if needed
    }

    @Benchmark
    public void generateKeyPair() throws JniException {
        paillier.generateKeyPair(keyBits);
    }

    @Benchmark
    public void encryptionFast() throws JniException {
        paillier.encryptFast(m1, keypair);
    }

    @Benchmark
    public void encrypt() throws JniException {
        paillier.encrypt(m1, publickey);
    }

    @Benchmark
    public void decryption() throws JniException {
        paillier.decrypt(c1, keypair);
    }

    @Benchmark
    public void add() throws JniException {
        paillier.add(c1, c2, publickey);
    }

    @Benchmark
    public void sub() throws JniException {
        paillier.sub(c1, c2, publickey);
    }

    @Benchmark
    public void scalaMul() throws JniException {
        paillier.scalaMul(v, c1, publickey);
    }

    public static void main(String[] args) throws Exception {
        Options opt = new OptionsBuilder().include(PaillierBenchmark.class.getSimpleName()).forks(1).build();
        new Runner(opt).run();
    }
}