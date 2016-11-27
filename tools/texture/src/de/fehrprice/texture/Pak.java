package de.fehrprice.texture;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;

public class Pak {

	public static final String[] textureFilesAll = { "default.dds", "dirt6_markings.dds", "grassdirt8.dds",
			"mars_6k_color.dds", "metal1.dds", "vac_00.dds", "vac_01.dds", "vac_02.dds", "vac_03.dds", "vac_04.dds",
			"vac_05.dds", "vac_06.dds", "vac_07.dds", "vac_08.dds", "vac_09.dds", "vac_10.dds", "vac_11.dds",
			"2create_brick_0001.dds", "met1.dds",};
	public static final String[] textureFilesSome = { "default.dds", "dirt6_markings.dds", };

	// public static final String pathPrepTexture = "../../data/texture/";
	public static final String pathPrepTexture = "dds_files/";
	public static final String pakName = "texture01.pak";
	public static final String pathPrepPak = "../../data/texture/";
	public static final long DirectoryEntrySize = 128;// 512;
	public static final long MAX_NAME_LENGTH = DirectoryEntrySize - 20;
	public static final String MAGIC = "SP12PAK0";
	public static final long MAGIC_LONG = 0x5350313250414B30L;

	// local class to hold all the data we need:
	private class PakEntry {
		public long len; // file length in bytes
		public long offset; // offset in pak - will be transferred to absolute
							// position in pak file on save
		public String name; // directory entry - may contain fake folder names
							// 'sub/t.dds'
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
		for (String filename : textureFilesAll) {
			pak.register(pathPrepTexture, filename);
		}
		pak.pakEntries
				.forEach(e -> System.out.println("Entry name/offset/len " + e.name + " " + e.offset + " " + e.len));
		pak.pack(pathPrepPak, pakName);
	}

	private void pack(String path, String name) {
		System.out.println(" write pk file: " + path + name);
		File file = new File(path + name);
		if (file.exists())
			file.delete();
		long completeDirectorySize = DirectoryEntrySize * pakEntries.size();
		// fix offsets:
		pakEntries.forEach(e -> e.offset += completeDirectorySize + 16);
		// pakEntries.forEach(e -> System.out.println("Entry name/offset/len " +
		// e.name + " " + e.offset + " " + e.len));
		try {
			DataOutputStream oos = new DataOutputStream(new BufferedOutputStream(new FileOutputStream(file)));
			// write MAGIC and number of entries as Longs, so that directory
			// starts at offset 16:
			writeLong(oos, Long.reverseBytes(MAGIC_LONG));
			writeLong(oos, pakEntries.size());
			pakEntries.forEach(e -> writeDirectoryEntry(oos, e));
			pakEntries.forEach(e -> copyFileContent(oos, e));
			oos.close();
		} catch (IOException e) {
			e.printStackTrace();
			fail("IO Error");
		}
		System.out.println("finished");
	}

	private Object copyFileContent(DataOutputStream oos, PakEntry e) {
		byte[] buffer = new byte[1024 * 64];
		int length;
		InputStream is = null;
		try {
			is = new BufferedInputStream(new FileInputStream(e.file));
			while ((length = is.read(buffer)) > 0) {
				oos.write(buffer, 0, length);
			}
			is.close();
		} catch (FileNotFoundException e1) {
			e1.printStackTrace();
			fail("IO error");
		} catch (IOException e1) {
			e1.printStackTrace();
			fail("IO error");
		}
		return null;
	}

	private Object writeDirectoryEntry(DataOutputStream oos, PakEntry e) {
		// long is 8 bytes
		writeLong(oos, e.offset);
		writeLong(oos, e.len);
		writeInt(oos, e.name.length());
		writeString(oos, e.name);
		// fill the rest with zero bytes:
		long rest = DirectoryEntrySize - 20 - e.name.length();
		while (rest > 0) {
			writeByte(oos, 0);
			rest--;
		}
		return null;
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
		long l = textureFile.length();
		long o = pakEntries.size() == 0 ? 0
				: pakEntries.get(pakEntries.size() - 1).offset + pakEntries.get(pakEntries.size() - 1).len;
		PakEntry pe = new PakEntry();
		pe.file = textureFile;
		pe.len = l;
		pe.offset = o;
		pe.name = textureFile.getName();
		// check filename length
		if (pe.name.length() > MAX_NAME_LENGTH)
			fail("filename too long for PAK directory: " + pe.name);
		// check character set:
		Pattern p = Pattern.compile("[^a-zA-Z0-9/._]");
		boolean invalid = p.matcher(pe.name).find();
		if (invalid)
			fail("filename contains forbidden chars: " + pe.name);
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

	private void writeString(DataOutputStream oos, String s) {
		// don't pay attention to unicode, just write the char bytes
		// will only work for clean ASCII values < 128
		for (int i = 0; i < s.length(); i++) {
			byte b = (byte) s.charAt(i);
			try {
				oos.writeByte(b);
			} catch (IOException e) {
				e.printStackTrace();
				fail("IO Error");
			}
		}
	}

	private void writeInt(DataOutputStream oos, int i) {
		try {
			oos.writeInt(Integer.reverseBytes(i));
		} catch (IOException e) {
			e.printStackTrace();
			fail("IO Error");
		}
	}

	private void writeLong(DataOutputStream oos, long l) {
		try {
			oos.writeLong(Long.reverseBytes(l));
		} catch (IOException e) {
			e.printStackTrace();
			fail("IO Error");
		}
	}

	private void writeByte(DataOutputStream oos, int b) {
		try {
			oos.writeByte(b);
		} catch (IOException e) {
			e.printStackTrace();
			fail("IO Error");
		}
	}

}
