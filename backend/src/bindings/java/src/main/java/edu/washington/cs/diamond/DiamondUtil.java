package edu.washington.cs.diamond;

import org.bytedeco.javacpp.*;
import org.bytedeco.javacpp.annotation.*;

/*
 * The StringVector class was copied from the file "videoInputLib.java" in javacpp-presents
 * on Github: https://github.com/bytedeco/javacpp-presets
 */

//@Platform(include={"<vector>", "<string>"})

public class DiamondUtil {

    @Name("std::vector<std::string>") public static class StringVector extends Pointer {
        static { Loader.load(); }
        /** Pointer cast constructor. Invokes {@link Pointer#Pointer(Pointer)}. */
        public StringVector(Pointer p) { super(p); }
        public StringVector(BytePointer ... array) { this(array.length); put(array); }
        public StringVector()       { allocate();  }
        public StringVector(long n) { allocate(n); }
        private native void allocate();
        private native void allocate(@Cast("size_t") long n);
        public native @Name("operator=") @ByRef StringVector put(@ByRef StringVector x);

        public native long size();
        public native void resize(@Cast("size_t") long n);

        @Index public native @StdString BytePointer get(@Cast("size_t") long i);
        public native StringVector put(@Cast("size_t") long i, BytePointer value);

        public StringVector put(BytePointer ... array) {
            if (size() != array.length) { resize(array.length); }
            for (int i = 0; i < array.length; i++) {
                put(i, array[i]);
            }
            return this;
        }
    }

}
