package de.fehrprice.texture;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

public class Pak {

	public static final String[] textureFilesAll = {
			"default.dds",
			"dirt6_markings.dds",
			"grassdirt8.dds",
			"mars_6k_color.dds",
			"metal1.dds",
			"vac_00.dds",
			"vac_01.dds",
			"vac_02.dds",
			"vac_03.dds",
			"vac_04.dds",
			"vac_05.dds",
			"vac_06.dds",
			"vac_07.dds",
			"vac_08.dds",
			"vac_09.dds",
			"vac_10.dds",
			"vac_11.dds",
	};
	public static final String[] textureFilesSome = {
			"default.dds",
			"dirt6_markings.dds",
	};
	
	//public static final String pathPrepTexture = "../../data/texture/";
	public static final String pathPrepTexture = "dds_files/";
	public static final String pakName = "texture01.pak";
	public static final String pathPrepPak = "../../data/texture/";
	
	// local class to hold all the data we need:
	private class PakEntry {
		public long len;	// file length in bytes
		public long offset;	// offset in pak - will be transferred to absolute position in pak file on save
		public String name; // directory entry - may contain fake folder names 'sub/t.dds'
		public File file;   
	}
	
	public List<PakEntry> pakEntries = new ArrayList<PakEntry>();

	/**
	 * Create pak file in data/texture folder content is hardcoded here
	 * 
	 * @param args
	 */
	public static void main(String[] args) {
		printCurrentWorkingDir();
		Pak pak = new Pak();
		for (String filename : textureFilesSome) {
			pak.register(pathPrepTexture, filename);
		}
		pak.pakEntries.forEach(e -> System.out.println("Entry name/offset/len " + e.name + " " + e.offset + " " + e.len));
		pak.pack(pathPrepPak, pakName);
	}

	private void pack(String path, String name) {
		System.out.println(" write pk file: " + path + name);
		File file = new File(path + name);
		if (file.exists()) file.delete();
/*		try {
			Files.write(Paths.get("c:/dev/metadata.txt"), out.toString().getBytes());
			System.out.println("written file to c:\\dev\\metadata.txt");
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
*/	}

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
		long l = textureFile.length();
		long o = pakEntries.size() == 0 ? 0 : pakEntries.get(pakEntries.size()-1).offset + pakEntries.get(pakEntries.size()-1).len;
		PakEntry pe = new PakEntry();
		pe.file = textureFile;
		pe.len = l;
		pe.offset = o;
		pe.name = textureFile.getName();
		pakEntries.add(pe);
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
