package de.fehrprice.texture;

import java.io.File;
import java.nio.file.Path;
import java.nio.file.Paths;

public class Pak {

	public static final String[] textureFiles = {
			"default.dds",
	};
	//public static final String pathPrepTexture = "../../data/texture/";
	public static final String pathPrepTexture = "dds_files/";
	public static final String pakName = "texture01.pak";
	public static final String pathPrepPak = "hugo";

	/**
	 * Create pak file in data/texture folder content is hardcoded here
	 * 
	 * @param args
	 */
	public static void main(String[] args) {
		printCurrentWorkingDir();
		Pak pak = new Pak();
		for (String filename : textureFiles) {
			pak.register(pathPrepTexture, filename);
		}
		pak.pack(pathPrepPak, pakName);
	}

	private void pack(String pathpreppak2, String pakname2) {

	}

	/**
	 * Register a file for inclusion in pak file. Reads length and stores in
	 * internal list.
	 * 
	 * @param pathpreptexture2
	 * @param filename
	 */
	private void register(String pathPrep, String filename) {
		File textureFile = new File(pathPrep + filename);
		if (!textureFile.exists()) {
			fail("input texture file " + textureFile + " does not exist");
		}
	}

	private void fail(String string) {
		System.out.println(string);
		System.exit(0);

	}

	private static void printCurrentWorkingDir() {
		Path currentRelativePath = Paths.get("");
		String s = currentRelativePath.toAbsolutePath().toString();
		System.out.println("relative paths relate to this: " + s);
	}
}
