package de.fehrprice.util;

import java.util.ArrayList;
import java.util.List;

import de.fehrprice.util.TestTree.Anything;

/**
 * ListTree - build tree structure from simple lists. Preserve order for added nodes on same level. No restructuring. No element deletion.
 *
 * @param <T>
 */
public class ListTree<T> {
	public class ListTreeNode<N> {
		N e = null;
		ListTreeNode<N> parent = null;
		List<ListTreeNode<N>> children = new ArrayList<>();
	}
	
	private ListTreeNode<T> root = null;

	// create new tree with empty root element
	public ListTree() {
		root = new ListTreeNode<T>();
		root.e = null;
	}
	
	// create new tree and set root element
	public ListTree(T e) {
		root = new ListTreeNode<T>();
		root.e = e;
	}
	
	// check if tree is empty (containes no elemnts)
	public boolean isEmpty() {
		return root.e == null && root.children.size() == 0;
	}
	
	// empty tree structure
	public void empty() {
		this.root = new ListTreeNode<T>();
	}

	/**
	 * Add element to parent node. Element will be appended to and of children list.
	 * @param node if node is null element will be appended under root node
	 * @param a
	 */
	public void add(ListTreeNode<T> node, T a) {
		// TODO Auto-generated method stub
		
	}
}

