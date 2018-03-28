package monosat;

import org.junit.Test;

import java.util.List;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * Tests the basic API functionality of bitvectors.
 * The goal of these tests is simply to make sure the JNI implementation works as expected,
 * but does not attempt to fully test the correctness of the underlying solver.
 */
public class BitVectorTest {

    @Test
    public void getBits() {
        Solver s = new Solver();
        BitVector bv = s.bv(4);
        List<Lit> bits = bv.getBits();
        assertEquals(bits.size(), 4);
        for (Lit l : bits) {
            assertEquals(s.solve(l), true);
            assertEquals(s.solve(l.negate()), true);
        }
    }

    @Test
    public void width() {
        Solver s = new Solver();
        BitVector bv = s.bv(4);
        assertEquals(bv.width(), 4);
        assertEquals(bv.size(), 4);
    }

    @Test
    public void gt() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = s.bv(4);
        Lit c = bv1.gt(bv2);
        assertEquals(s.solve(c), true);
        long v1 = s.getValue(bv1);
        long v2 = s.getValue(bv2);
        assertTrue("BV1 > BV2", v1 > v2);
        assertEquals(s.solve(c.negate()), true);
        v1 = s.getValue(bv1);
        v2 = s.getValue(bv2);
        assertTrue("BV1 <= BV2", v1 <= v2);
    }

    @Test
    public void geq() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = s.bv(4);
        Lit c = bv1.geq(bv2);
        assertEquals(s.solve(c), true);
        long v1 = s.getValue(bv1);
        long v2 = s.getValue(bv2);
        assertTrue("BV1 >= BV2", v1 >= v2);
        assertEquals(s.solve(c.negate()), true);
        v1 = s.getValue(bv1);
        v2 = s.getValue(bv2);
        assertTrue("BV1 < BV2", v1 < v2);
    }

    @Test
    public void lt() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = s.bv(4);
        Lit c = bv1.lt(bv2);
        assertEquals(s.solve(c), true);
        long v1 = s.getValue(bv1);
        long v2 = s.getValue(bv2);
        assertTrue("BV1 < BV2", v1 < v2);
        assertEquals(s.solve(c.negate()), true);
        v1 = s.getValue(bv1);
        v2 = s.getValue(bv2);
        assertTrue("BV1 >= BV2", v1 >= v2);
    }

    @Test
    public void leq() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = s.bv(4);
        Lit c = bv1.leq(bv2);
        assertEquals(s.solve(c), true);
        long v1 = s.getValue(bv1);
        long v2 = s.getValue(bv2);
        assertTrue("BV1 <= BV2", v1 <= v2);
        assertEquals(s.solve(c.negate()), true);
        v1 = s.getValue(bv1);
        v2 = s.getValue(bv2);
        assertTrue("BV1 > BV2", v1 > v2);
    }

    @Test
    public void neq() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = s.bv(4);
        Lit c = bv1.neq(bv2);
        assertEquals(s.solve(c), true);
        long v1 = s.getValue(bv1);
        long v2 = s.getValue(bv2);
        assertTrue("BV1 != BV2", v1 != v2);
        assertEquals(s.solve(c.negate()), true);
        v1 = s.getValue(bv1);
        v2 = s.getValue(bv2);
        assertTrue("BV1 == BV2", v1 == v2);
    }

    @Test
    public void eq() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = s.bv(4);
        Lit c = bv1.eq(bv2);
        assertEquals(s.solve(c), true);
        long v1 = s.getValue(bv1);
        long v2 = s.getValue(bv2);
        assertTrue("BV1 == BV2", v1 == v2);
        assertEquals(s.solve(c.negate()), true);
        v1 = s.getValue(bv1);
        v2 = s.getValue(bv2);
        assertTrue("BV1 != BV2", v1 != v2);
    }


    @Test
    public void slice() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = bv1.slice(1, 3);
        assertEquals(bv2.width(), 2);
        assertEquals(s.solve(bv1.get(1), bv2.get(0).negate()), false);
    }

    @Test
    public void append() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = s.bv(3);
        BitVector bv3 = bv1.append(bv2);
        assertEquals(bv3.width(), 7);
        assertEquals(s.solve(bv1.get(1), bv3.get(1)), true);
        assertEquals(s.solve(bv1.get(1), bv3.get(1).negate()), false);
        assertEquals(s.solve(bv2.get(1), bv3.get(5)), true);
        assertEquals(s.solve(bv2.get(1), bv3.get(5).negate()), false);
    }

    @Test
    public void not() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = bv1.not();
        for (int i = 0; i < bv1.size(); i++) {
            assertEquals(s.solve(bv1.get(i), bv2.get(i)), false);
            assertEquals(s.solve(bv1.get(i), bv2.get(i).negate()), true);
        }
    }

    @Test
    public void and() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = s.bv(4);
        BitVector bv3 = bv1.and(bv2);
        for (int i = 0; i < bv1.size(); i++) {
            assertEquals(s.solve(bv1.get(i), bv2.get(i), bv3.get(i)), true);
            assertEquals(s.solve(bv1.get(i), bv2.get(i), bv3.get(i).negate()), false);
            assertEquals(s.solve(bv1.get(i), bv2.get(i).negate(), bv3.get(i)), false);
            assertEquals(s.solve(bv1.get(i).negate(), bv2.get(i).negate(), bv3.get(i)), false);
            assertEquals(s.solve(bv1.get(i).negate(), bv2.get(i), bv3.get(i)), false);
        }
    }

    @Test
    public void nand() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = s.bv(4);
        BitVector bv3 = bv1.nand(bv2);
        for (int i = 0; i < bv1.size(); i++) {
            assertEquals(s.solve(bv1.get(i), bv2.get(i), bv3.get(i)), false);
            assertEquals(s.solve(bv1.get(i), bv2.get(i), bv3.get(i).negate()), true);
            assertEquals(s.solve(bv1.get(i), bv2.get(i).negate(), bv3.get(i)), true);
            assertEquals(s.solve(bv1.get(i).negate(), bv2.get(i).negate(), bv3.get(i)), true);
            assertEquals(s.solve(bv1.get(i).negate(), bv2.get(i), bv3.get(i)), true);
        }
    }

    @Test
    public void or() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = s.bv(4);
        BitVector bv3 = bv1.or(bv2);
        for (int i = 0; i < bv1.size(); i++) {
            assertEquals(s.solve(bv1.get(i), bv2.get(i), bv3.get(i)), true);
            assertEquals(s.solve(bv1.get(i), bv2.get(i), bv3.get(i).negate()), false);
            assertEquals(s.solve(bv1.get(i), bv2.get(i).negate(), bv3.get(i)), true);
            assertEquals(s.solve(bv1.get(i).negate(), bv2.get(i).negate(), bv3.get(i)), false);
            assertEquals(s.solve(bv1.get(i).negate(), bv2.get(i), bv3.get(i)), true);
        }
    }

    @Test
    public void nor() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = s.bv(4);
        BitVector bv3 = bv1.nor(bv2);
        for (int i = 0; i < bv1.size(); i++) {
            assertEquals(s.solve(bv1.get(i), bv2.get(i), bv3.get(i)), false);
            assertEquals(s.solve(bv1.get(i), bv2.get(i), bv3.get(i).negate()), true);
            assertEquals(s.solve(bv1.get(i), bv2.get(i).negate(), bv3.get(i)), false);
            assertEquals(s.solve(bv1.get(i).negate(), bv2.get(i).negate(), bv3.get(i)), true);
            assertEquals(s.solve(bv1.get(i).negate(), bv2.get(i), bv3.get(i)), false);
        }
    }

    @Test
    public void xor() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = s.bv(4);
        BitVector bv3 = bv1.xor(bv2);
        for (int i = 0; i < bv1.size(); i++) {
            assertEquals(s.solve(bv1.get(i), bv2.get(i), bv3.get(i)), false);
            assertEquals(s.solve(bv1.get(i), bv2.get(i), bv3.get(i).negate()), true);
            assertEquals(s.solve(bv1.get(i), bv2.get(i).negate(), bv3.get(i)), true);
            assertEquals(s.solve(bv1.get(i).negate(), bv2.get(i).negate(), bv3.get(i)), false);
            assertEquals(s.solve(bv1.get(i).negate(), bv2.get(i), bv3.get(i)), true);
        }
    }

    @Test
    public void xnor() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = s.bv(4);
        BitVector bv3 = bv1.xnor(bv2);
        for (int i = 0; i < bv1.size(); i++) {
            assertEquals(s.solve(bv1.get(i), bv2.get(i), bv3.get(i)), true);
            assertEquals(s.solve(bv1.get(i), bv2.get(i), bv3.get(i).negate()), false);
            assertEquals(s.solve(bv1.get(i), bv2.get(i).negate(), bv3.get(i)), false);
            assertEquals(s.solve(bv1.get(i).negate(), bv2.get(i).negate(), bv3.get(i)), true);
            assertEquals(s.solve(bv1.get(i).negate(), bv2.get(i), bv3.get(i)), false);
        }
    }

    @Test
    public void add() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = s.bv(4);
        BitVector bv3 = bv1.add(bv2);
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                assertEquals(s.solve(bv1.eq(i), bv2.eq(j)), true);
                assertEquals(s.getValue(bv1), i);
                assertEquals(s.getValue(bv2), j);
                assertEquals(s.getValue(bv3), i + j);
                assertEquals(s.solve(bv1.eq(i), bv2.eq(j), bv3.neq(i + j)), false);
            }
        }
    }

    @Test
    public void subtract() {
        Solver s = new Solver();
        BitVector bv1 = s.bv(4);
        BitVector bv2 = s.bv(4);
        BitVector bv3 = bv1.subtract(bv2);
        for (int i = 0; i < 7; i++) {
            for (int j = 0; j <= i; j++) {
                assertEquals(s.solve(bv1.eq(i), bv2.eq(j)), true);
                assertEquals(s.getValue(bv1), i);
                assertEquals(s.getValue(bv2), j);
                assertEquals(s.getValue(bv3), i - j);
                assertEquals(s.solve(bv1.eq(i), bv2.eq(j), bv3.neq(i - j)), false);
            }
        }
    }
}