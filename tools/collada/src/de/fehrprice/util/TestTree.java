package de.fehrprice.util;

import static org.junit.Assert.assertEquals;
import static org.junit.jupiter.api.Assertions.*;

import java.util.stream.Stream;
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
	
	@Test
	void testTree() {
		int num = 1; // count number of created elements
		ListTree<Anything> tree = new ListTree<>();
		assertTrue(tree.isEmpty());
		//fail("Not yet implemented");
		
		Anything a = new Anything();
		a.num = num++;
		ListTreeNode<Anything> e1 = tree.add(null, a);
		a = new Anything();
		a.num = num++;
		tree.add(null, a); // 2nd child of root
		
		a = new Anything();
		a.num = num++;
		tree.add(e1, a); // one child at first element under root 
		
		//List<Anything> l = tree.flattenedTopDown().map(ListTree.ListTreeNode::get).filter(Objects::nonNull).collect(Collectors.toList());
		List<Anything> l = tree.getElementsTopDown();
		assertTrue(l.size() == 3);
		
		String s = "";
		for (Anything any : l) {
			s += any.num + " ";
		}
		System.out.println(s);
		assertEquals("1 3 2 ", s);
		
		List<ListTreeNode<Anything>> ln = tree.getNodesTopDown();
		for (ListTreeNode<Anything> any : ln) {
			//String spaces = String.format("%1$"+(any.layer)+"s", "");
			int count = any.layer+1;
			String spaces = String.format("%"+ count +"s", " ");
			//String spaces = String.format("%1$"+count+"s", "");
			String v = any.e == null ? "root" : ""+any.e.num;
			System.out.println(spaces + v);
		}
		
		tree.printNodesTopDown(Anything::print);
	}

}
