package de.fehrprice.util;

import static org.junit.Assert.assertEquals;
import static org.junit.jupiter.api.Assertions.*;

import java.util.stream.Stream;

import javax.naming.OperationNotSupportedException;

import java.util.List;
import java.util.Objects;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;

import de.fehrprice.util.ListTree.ListTreeNode;

/**
 * 
 *
 */
class TestTree {

	public class Anything {
		public int num;
		public String name; 
		public String print() {
			return "" + num;
		}
	}

	public static String getFlatStringFromIds(List<Anything> l) {
		String s = "";
		for (Anything any : l) {
			s += any.num + " ";
		}
		return s;
	}
	
	@Test
	void testTree() throws OperationNotSupportedException {
		int num = 1; // count number of created elements
		ListTree<Anything> tree = new ListTree<>();
		assertTrue(tree.isEmpty());
		assertTrue(tree.size() == 0);
		//fail("Not yet implemented");
		
		Anything a = new Anything();
		a.num = num++;
		ListTreeNode<Anything> e1 = tree.add(null, a); // now a is root
		final Anything x = new Anything();
		x.num = num++;
		assertThrows(OperationNotSupportedException.class, () -> {
			tree.add(null, x); // 2nd child of root
		  });
		
		var b1 = new Anything();
		b1.num = num++;
		var b1Node = tree.add(e1, b1); // one child at first element under root
		assertEquals(2, tree.size());
		
		var b2 = new Anything();
		b2.num = num++;
		tree.add(e1, b2); // one child at first element under root
		assertEquals(3, tree.size());

		//List<Anything> l = tree.flattenedTopDown().map(ListTree.ListTreeNode::get).filter(Objects::nonNull).collect(Collectors.toList());
		List<Anything> l = tree.getElementsTopDown();
		assertEquals(3, l.size());
		
		String s = getFlatStringFromIds(l);
		//System.out.println(s);
		assertEquals("1 3 4 ", s);
		
		// now add child to b1:
		Anything c = new Anything();
		c.num = num++;
		tree.add(b1Node, c);
		l = tree.getElementsTopDown();
		s = getFlatStringFromIds(l);
		assertEquals("1 3 5 4 ", s);
		
		tree.printNodesTopDown(Anything::print);
	}

}
