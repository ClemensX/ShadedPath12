package de.fehrprice.util;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.function.Consumer;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import de.fehrprice.util.TestTree.Anything;

/**
 * ListTree - build tree structure from simple lists. Preserve order for added
 * nodes on same level. No restructuring. No element deletion.
 *
 * @param <T>
 */
public class ListTree<T> {
	public static class ListTreeNode<N> {
		N e = null;
		int layer; // root is layer 0
		ListTreeNode<N> parent = null;
		List<ListTreeNode<N>> children = new ArrayList<>();
		public Stream<ListTreeNode<N>> flattenedTopDown() {
			return Stream.concat(Stream.of(this), children.stream().flatMap(ListTreeNode::flattenedTopDown));
		}
		public N get() {
			return e;
		}
	}

	private ListTreeNode<T> root = null;

	// create new tree with empty root element
	public ListTree() {
		root = new ListTreeNode<T>();
		root.e = null;
		root.layer = 0;
	}

	// create new tree and set root element
	public ListTree(T e) {
		super();
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
	
	public ListTreeNode<T> getRoot() {
		return root;
	}

	/**
	 * Add element to parent node. Element will be appended to and of children list.
	 * 
	 * @param parent if node is null element will be appended under root node
	 * @param a
	 * @return tree node at insertion point
	 */
	public ListTreeNode<T> add(ListTreeNode<T> parent, T a) {
		if (parent == null) {
			parent = getRoot();
		}
		ListTreeNode<T> n = new ListTreeNode<T>();
		n.e = a;
		n.parent = parent;
		n.layer = parent.layer + 1;
		parent.children.add(n);
		return n;
	}

	public Stream<ListTreeNode<T>> flattenedTopDown() {
		return root.flattenedTopDown();
	}
	
	public List<T> getElementsTopDown() {
		return flattenedTopDown().map(ListTree.ListTreeNode::get).filter(Objects::nonNull).collect(Collectors.toList());
	}

	public List<ListTreeNode<T>> getNodesTopDown() {
		return flattenedTopDown().filter(Objects::nonNull).collect(Collectors.toList());
	}

	public void printNodesTopDown(Consumer<T> printMethod) {
		//t -> { printMethod.accept(t); }
		
	}
}
