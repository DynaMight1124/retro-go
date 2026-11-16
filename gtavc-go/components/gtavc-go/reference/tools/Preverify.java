import jdk.internal.org.objectweb.asm.*;
import java.io.*;
import java.net.*;
import java.nio.file.*;
import java.util.*;

/**
 * Turns javac output into CLDC-ready classes:
 *  - class version 46 (Java 1.2) which every J2ME VM accepts
 *  - StringBuilder -> StringBuffer (CLDC has no StringBuilder)
 *  - computes CLDC "StackMap" attributes (what the WTK preverifier does)
 *  - verifies the code only touches CLDC 1.1 / MIDP 2.0 API
 *
 * usage: Preverify <rawClassesDir> <outDir> <extraClasspathDir>
 */
public class Preverify {
    static URLClassLoader LOADER;
    static int errors = 0;

    static final String[] OK_PREFIX = {
        "java/lang/Object", "java/lang/String", "java/lang/StringBuffer", "java/lang/Math", "java/lang/System",
        "java/lang/Thread", "java/lang/Runnable", "java/lang/Integer", "java/lang/Short", "java/lang/Byte",
        "java/lang/Character", "java/lang/Boolean", "java/lang/Long", "java/lang/Float", "java/lang/Double",
        "java/lang/Throwable", "java/lang/Exception", "java/lang/RuntimeException", "java/lang/Error",
        "java/lang/Class", "java/lang/Runtime", "java/lang/OutOfMemoryError", "java/lang/InterruptedException",
        "java/lang/NullPointerException", "java/lang/IllegalArgumentException", "java/lang/ArithmeticException",
        "java/lang/IndexOutOfBoundsException", "java/lang/ArrayIndexOutOfBoundsException",
        "java/lang/ClassNotFoundException", "java/lang/InstantiationException", "java/lang/IllegalAccessException",
        "java/lang/NumberFormatException", "java/lang/SecurityException", "java/lang/IllegalStateException",
        "java/lang/ArrayStoreException", "java/lang/ClassCastException", "java/lang/NegativeArraySizeException",
        "java/lang/StringIndexOutOfBoundsException", "java/lang/VirtualMachineError", "java/lang/StackOverflowError",
        "java/util/Vector", "java/util/Hashtable", "java/util/Enumeration", "java/util/Stack", "java/util/Random",
        "java/util/Timer", "java/util/TimerTask", "java/util/Date", "java/util/Calendar", "java/util/EmptyStackException",
        "java/util/NoSuchElementException",
        "java/io/InputStream", "java/io/DataInputStream", "java/io/ByteArrayInputStream", "java/io/OutputStream",
        "java/io/DataOutputStream", "java/io/ByteArrayOutputStream", "java/io/IOException", "java/io/PrintStream",
        "java/io/EOFException", "java/io/DataInput", "java/io/DataOutput",
        "javax/microedition/"
    };
    // methods that exist on desktop Java but NOT in CLDC 1.1
    static final String[] BAD_METHODS = {
        "java/lang/String.contains", "java/lang/String.format", "java/lang/String.isEmpty", "java/lang/String.split",
        "java/lang/String.matches", "java/lang/String.replaceAll", "java/lang/String.replaceFirst",
        "java/lang/String.join", "java/lang/String.chars", "java/lang/String.strip", "java/lang/String.repeat",
        "java/lang/Integer.valueOf(I)", "java/lang/Long.valueOf(J)", "java/lang/Boolean.valueOf(Z)",
        "java/lang/Short.valueOf(S)", "java/lang/Byte.valueOf(B)", "java/lang/Character.valueOf(C)",
        "java/lang/Integer.compare", "java/lang/Integer.signum", "java/lang/Integer.bitCount",
        "java/lang/Math.random", "java/lang/Math.round", "java/lang/Math.atan2", "java/lang/Math.atan", "java/lang/Math.asin", "java/lang/Math.acos", "java/lang/Math.pow", "java/lang/Math.exp", "java/lang/Math.log", "java/lang/Math.floorDiv", "java/lang/Math.floorMod",
        "java/lang/Math.hypot", "java/lang/Math.cbrt", "java/lang/Math.signum",
        "java/lang/StringBuffer.delete", "java/lang/StringBuffer.deleteCharAt", "java/lang/StringBuffer.replace",
        "java/lang/StringBuffer.substring", "java/lang/StringBuffer.indexOf", "java/lang/StringBuffer.appendCodePoint",
        "java/lang/System.nanoTime", "java/lang/System.lineSeparator", "java/lang/System.getenv",
        "java/util/Random.nextInt(I)", "java/util/Random.nextBoolean", "java/util/Random.nextGaussian",
        "java/util/Vector.add", "java/util/Vector.get", "java/util/Vector.remove(", "java/util/Vector.iterator",
        "java/util/Vector.stream", "java/util/Vector.forEach", "java/util/Vector.subList", "java/util/Vector.clone",
        "java/util/Hashtable.entrySet", "java/util/Hashtable.keySet", "java/util/Hashtable.values",
        "java/lang/Thread.getId", "java/lang/Thread.setName", "java/lang/Thread.isInterrupted"
    };

    static boolean okOwner(String o) {
        while (o.startsWith("[")) o = o.substring(1);
        if (o.startsWith("L") && o.endsWith(";")) o = o.substring(1, o.length() - 1);
        if (o.length() == 1) return true; // primitive array element
        for (int i = 0; i < OK_PREFIX.length; i++) {
            if (o.equals(OK_PREFIX[i]) || (OK_PREFIX[i].endsWith("/") && o.startsWith(OK_PREFIX[i]))) return true;
        }
        return !o.startsWith("java/") && !o.startsWith("javax/") && !o.startsWith("jdk/") && !o.startsWith("sun/");
    }

    static String fix(String s) {
        return s == null ? null : s.replace("java/lang/StringBuilder", "java/lang/StringBuffer");
    }

    static void err(String where, String msg) {
        errors++;
        System.err.println("CLDC ERROR in " + where + ": " + msg);
    }

    public static void main(String[] a) throws Exception {
        File in = new File(a[0]), out = new File(a[1]);
        ArrayList urls = new ArrayList();
        urls.add(in.toURI().toURL());
        for (int i = 2; i < a.length; i++) urls.add(new File(a[i]).toURI().toURL());
        LOADER = new URLClassLoader((URL[]) urls.toArray(new URL[0]), Preverify.class.getClassLoader());
        int n = walk(in, in, out);
        System.out.println("preverified " + n + " classes, " + errors + " CLDC problems");
        if (errors > 0) System.exit(1);
    }

    static int walk(File root, File f, File out) throws Exception {
        int n = 0;
        if (f.isDirectory()) {
            File[] k = f.listFiles();
            Arrays.sort(k);
            for (int i = 0; i < k.length; i++) n += walk(root, k[i], out);
        } else if (f.getName().endsWith(".class")) {
            String rel = root.toPath().relativize(f.toPath()).toString();
            byte[] b = Files.readAllBytes(f.toPath());
            byte[] r = process(b, rel);
            File o = new File(out, rel);
            o.getParentFile().mkdirs();
            Files.write(o.toPath(), r);
            n++;
        }
        return n;
    }

    static byte[] process(byte[] in, final String where) {
        ClassReader cr = new ClassReader(in);
        ClassWriter cw = new ClassWriter(ClassWriter.COMPUTE_FRAMES) {
            protected String getCommonSuperClass(String t1, String t2) {
                try {
                    Class c = Class.forName(t1.replace('/', '.'), false, LOADER);
                    Class d = Class.forName(t2.replace('/', '.'), false, LOADER);
                    if (c.isAssignableFrom(d)) return t1;
                    if (d.isAssignableFrom(c)) return t2;
                    if (c.isInterface() || d.isInterface()) return "java/lang/Object";
                    do { c = c.getSuperclass(); } while (!c.isAssignableFrom(d));
                    return c.getName().replace('.', '/');
                } catch (Exception e) {
                    throw new RuntimeException(e.toString());
                }
            }
        };
        ClassVisitor cv = new ClassVisitor(Opcodes.ASM8, cw) {
            public void visit(int v, int acc, String name, String sig, String sup, String[] ifs) {
                if ((acc & Opcodes.ACC_ENUM) != 0) err(where, "enum");
                if (!okOwner(fix(sup))) err(where, "superclass " + sup);
                super.visit(Opcodes.V1_2, acc, name, null, fix(sup), ifs);
            }
            public FieldVisitor visitField(int acc, String n, String d, String s, Object val) {
                return super.visitField(acc, n, fix(d), null, val);
            }
            public MethodVisitor visitMethod(int acc, final String mn, String desc, String sig, String[] ex) {
                final String loc = where + "." + mn;
                MethodVisitor mv = super.visitMethod(acc, mn, fix(desc), null, ex);
                return new MethodVisitor(Opcodes.ASM8, mv) {
                    public void visitMethodInsn(int op, String owner, String name, String d, boolean itf) {
                        owner = fix(owner);
                        d = fix(d);
                        if (!okOwner(owner)) err(loc, "class " + owner + " (" + name + ")");
                        String key = owner + "." + name;
                        for (int i = 0; i < BAD_METHODS.length; i++) {
                            String bm = BAD_METHODS[i];
                            if (bm.indexOf('(') > 0) {
                                if (key.equals(bm.substring(0, bm.indexOf('('))) && d.startsWith(bm.substring(bm.indexOf('(')) ) ) err(loc, "method not in CLDC: " + owner + "." + name + d);
                            } else if (key.equals(bm)) err(loc, "method not in CLDC: " + owner + "." + name + d);
                        }
                        if (itf && op == Opcodes.INVOKESTATIC) err(loc, "static interface method");
                        super.visitMethodInsn(op, owner, name, d, itf);
                    }
                    public void visitFieldInsn(int op, String owner, String name, String d) {
                        owner = fix(owner);
                        if (!okOwner(owner)) err(loc, "field of " + owner + "." + name);
                        super.visitFieldInsn(op, owner, name, fix(d));
                    }
                    public void visitTypeInsn(int op, String t) {
                        t = fix(t);
                        if (!okOwner(t)) err(loc, "type " + t);
                        super.visitTypeInsn(op, t);
                    }
                    public void visitLdcInsn(Object c) {
                        if (c instanceof Type) err(loc, "class literal (needs class version 49)");
                        super.visitLdcInsn(c);
                    }
                    public void visitInvokeDynamicInsn(String n, String d, Handle h, Object... args) {
                        err(loc, "invokedynamic/lambda");
                    }
                    public void visitMultiANewArrayInsn(String d, int dims) { super.visitMultiANewArrayInsn(fix(d), dims); }
                    public void visitLocalVariable(String n, String d, String s, Label a, Label b, int i) { }
                    public void visitTryCatchBlock(Label s, Label e, Label h, String t) { super.visitTryCatchBlock(s, e, h, fix(t)); }
                };
            }
        };
        cr.accept(cv, ClassReader.SKIP_FRAMES | ClassReader.SKIP_DEBUG);
        return cw.toByteArray();
    }
}
