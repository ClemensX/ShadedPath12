package de.fehrprice.util;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.naming.OperationNotSupportedException;

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

		public ListTreeNode<N> getParent() {
			return parent;
		}

		public List<ListTreeNode<N>> getChildren() {
			return children;
		}
	}

	private ListTreeNode<T> root = null;

	private int numElements = 0; // keep track of insertions

	public int size() {
		return numElements;
	}

	// create new tree with empty root element
	public ListTree() {
		root = null;
		numElements = 0;
	}

	// create new tree and set root element
	public ListTree(T e) throws OperationNotSupportedException {
		super();
		add(null, e);
	}

	// check if tree is empty (containes no elemnts)
	public boolean isEmpty() {
		return root == null;
	}

	// empty tree structure
	public void empty() {
		this.root = null;
	}

	public ListTreeNode<T> getRoot() {
		return root;
	}

	/**
	 * Add element to parent node. Element will be appended to end of children list.
	 * Adding more than one root node will result in Exception
	 * 
	 * @param parent node. if null element will be root node
	 * @param a
	 * @return tree node at insertion point
	 * @throws OperationNotSupportedException
	 */
	public ListTreeNode<T> add(ListTreeNode<T> parent, T a) throws OperationNotSupportedException {
		if (parent == null) {
			if (root != null) {
				throw new OperationNotSupportedException("Adding more than one root not is not allowed.");
			}
		}
		ListTreeNode<T> n = new ListTreeNode<T>();
		n.e = a;
		n.parent = parent;
		if (parent == null) {
			// first node is root node:
			n.layer = 0;
			root = n;
		} else {
			n.layer = parent.layer + 1;
			parent.children.add(n);
		}
		numElements++;
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

	/**
	 * Print a single string with spaces in front. Number of spaces could e.g.
	 * resemble the depth of a tree structure. Util method for walking a tree
	 * structure and pretty print its content.
	 * 
	 * @param layerIndentation depth of this element a.k.a. number of spaces to
	 *                         prepend
	 * @param text             to print after spaces
	 */
	public static void printIndented(int layerIndentation, String text) {
		String spaces;
		if (layerIndentation <= 0) {
			spaces = "";
		} else {
			spaces = String.format("%" + layerIndentation + "s", " ");
		}
		String s = text == null ? "" : text;
		System.out.println(spaces + s);
	}

	/**
	 * Print list of ListTreeNodes with indentation according to depth in tree. The
	 * root node is printed as 'root' if it does not contain an element.
	 * 
	 * @param nl          List<T> of nodes to print
	 * @param printMethod functional parameter: member method of T to be used for
	 *                    getting the string to print
	 */
	public void printNodes(List<ListTreeNode<T>> nl, Function<T, String> printMethod) {
		nl.forEach(t -> {
			String s = t.e == null ? "root" : printMethod.apply(t.e);
			printIndented(t.layer, s);
		});
	}

	/**
	 * Print nodes of a tree in a top down fashion. Each element is indented
	 * according to its depth in the tree.
	 * 
	 * @param printMethod functional parameter: member method of T to be used for
	 *                    getting the string to print
	 */
	public void printNodesTopDown(Function<T, String> printMethod) {
		List<ListTreeNode<T>> nl = getNodesTopDown();
		printNodes(nl, printMethod);
	}
}
