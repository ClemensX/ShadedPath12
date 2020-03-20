package de.fehrprice.util;

import static org.junit.jupiter.api.Assertions.*;

import org.junit.jupiter.api.Test;

/**
 * 
 *
 */
class TestTree {

	public class Anything {
		public int num;
		public String name; 
	}
	
	@Test
	void testTree() {
		int num = 1; // count number of created elements
		ListTree<Anything> tree = new ListTree<>();
		assertTrue(tree.isEmpty());
		//fail("Not yet implemented");
		
		Anything a = new Anything();
		a.num = num++;
		
		tree.add(null, a);
	}

}
