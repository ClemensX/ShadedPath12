/*
 * ColladaImport - read collada file and output custom binary format .b
 * 
 * (c) copyright 2013 Clemens Fehr - All rights reserved
 */
package de.fehrprice.collada;

import java.io.BufferedOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.Set;
import java.util.StringTokenizer;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import java.util.Arrays;

public class ColladaImport {

    public static int FPS = 24;  // we want to have frame numbers for D3D, as this setting is not exported via collada we use global const
    public static String filename;
    public static String outdir = "";
    public static boolean UV_DuplicateVertex = true;  // enable vertex duplication if needed for multiple UV coords on one vertex
    public static boolean fakeTextureMapping = false;  // set to true if no UV mapping available to fake texture coord

    public static boolean discardBoneInfluenceGreater4 = true;  // bone influence > 4 will be discarded instead of failed assertion
	private static boolean fixaxis = false;

    /**
     * @param args
     */
    public static void main(String[] args) {
        ColladaImport ci = new ColladaImport();
        if (args.length < 1) {
            System.out.println("missing parameter: filname of collada xml");
            usage();
            System.exit(0);
        }
        for (int i = 0; i < args.length; i++) {
            filename = args[args.length-1]; // collada input filename is last param
            if (args[i].startsWith("-outdir")) {
            	if (args.length < i+2) {
                    System.out.println("missing parameter: output directory or input filname of collada xml");
                    usage();
                    System.exit(0);
            	}
                outdir = args[i+1];
                File dir = new File(outdir);
                if (!dir.exists()) {
                    failUsage("Output directory " + outdir + " does not exist");
                }
            }        	
            if (args[i].startsWith("-fixaxis")) {
            	fixaxis  = true;
            	System.out.println(" Conversion from Blender coordinate system to SP engine enabled!");
            }        	
        }
        try {

             ci.importCollada(filename);
//            ci.importCollada("E:\\dev\\3d\\shaded2.dae");
//            ci.importCollada("E:\\dev\\3d\\path.dae");
//            ci.importCollada("E:\\dev\\3d\\worm5.dae");
//            ci.importCollada("E:\\dev\\3d\\house4_anim.dae");
//            ci.importCollada("E:\\dev\\3d\\CartoonBobcat(BLEND)\\CartoonBobcat2.dae");
//            ci.importCollada("E:\\dev\\3d\\CartoonBobcat(BLEND)\\untitled.dae");
//            ci.importCollada("E:\\dev\\3d\\sun.dae");
//            ci.importCollada("E:\\dev\\3d\\joint4_anim.dae");
//            ci.importCollada("E:\\dev\\3d\\joint5_anim.dae");

        } catch (ParserConfigurationException e) {
            System.out.println(e);
            e.printStackTrace();
        } catch (SAXException e) {
            System.out.println(e);
            e.printStackTrace();
        } catch (IOException e) {
            System.out.println(e);
            e.printStackTrace();
        }
    }

    private class D3DVertex {
        // position
        float x,y,z,a;
        // normal
        float nx, ny, nz;
        // texcoord
        float cu, cv;
        BoneWeight[] weight = null;

        
        private ColladaImport getOuterType() {
            return ColladaImport.this;
        }


        @Override
        public int hashCode() {
            final int prime = 31;
            int result = 1;
            result = prime * result + getOuterType().hashCode();
            result = prime * result + Float.floatToIntBits(a);
            result = prime * result + Float.floatToIntBits(cu);
            result = prime * result + Float.floatToIntBits(cv);
            result = prime * result + Float.floatToIntBits(nx);
            result = prime * result + Float.floatToIntBits(ny);
            result = prime * result + Float.floatToIntBits(nz);
            result = prime * result + Arrays.hashCode(weight);
            result = prime * result + Float.floatToIntBits(x);
            result = prime * result + Float.floatToIntBits(y);
            result = prime * result + Float.floatToIntBits(z);
            return result;
        }


        @Override
        public boolean equals(Object obj) {
            if (this == obj)
                return true;
            if (obj == null)
                return false;
            if (getClass() != obj.getClass())
                return false;
            D3DVertex other = (D3DVertex) obj;
            if (!getOuterType().equals(other.getOuterType()))
                return false;
            if (Float.floatToIntBits(a) != Float.floatToIntBits(other.a))
                return false;
            if (Float.floatToIntBits(cu) != Float.floatToIntBits(other.cu))
                return false;
            if (Float.floatToIntBits(cv) != Float.floatToIntBits(other.cv))
                return false;
            if (Float.floatToIntBits(nx) != Float.floatToIntBits(other.nx))
                return false;
            if (Float.floatToIntBits(ny) != Float.floatToIntBits(other.ny))
                return false;
            if (Float.floatToIntBits(nz) != Float.floatToIntBits(other.nz))
                return false;
            if (!Arrays.equals(weight, other.weight))
                return false;
            if (Float.floatToIntBits(x) != Float.floatToIntBits(other.x))
                return false;
            if (Float.floatToIntBits(y) != Float.floatToIntBits(other.y))
                return false;
            if (Float.floatToIntBits(z) != Float.floatToIntBits(other.z))
                return false;
            return true;
        }
    }
    
    private class D3DMeshdata {
        int numVerts;
        float[] verts;
        int numFaces;
        int numTexcoords;
        float[] texcoords;
        int numIndexes;
        List<Integer> indexBuffer;
        List<Integer> texIndexBuffer;
        List<Integer> normalIndexBuffer;
        int numNormals;
        float[] normals;
        public ArrayList<D3DVertex> d3dVertexList;
        public String geometryId;
        public String meshName;
    }
    
    private class Animation {
        // channel
        String target;
        String source;
        // sampler, use semantic string as key
        //Map<String,Object> sampler = new HashMap<String,Object>();
        List<BezTriple> beziers;
    }
    
    private class SkinnedAnimation {
        // channel
        //String target;
        //String source;
        // sampler, use semantic string as key
        //Map<String,Object> sampler = new HashMap<String,Object>();
        //List<BezTriple> beziers;
        public Float[] bindPoseMatrix;
        public List<Joint> joints = new ArrayList<Joint>();
        public List<BoneWeight[]> weights = new ArrayList<BoneWeight[]>();
        public String meshName;
        public String name;
        public List<Bone> bones;
    }
    
    private class Joint {
        String name;
        Float[] invBindMatrix;
    }
    
    private void importCollada(String filename) throws ParserConfigurationException, SAXException, IOException {
        // create output file name:
        File infile = new File(filename);
        String outfileName = infile.getName();
        int dotPos = outfileName.indexOf('.');
        if (dotPos >= 0) {
            outfileName = outfileName.substring(0, dotPos) + ".b";
        } else {
            outfileName = outfileName + ".b";
        }
        File outfile = new File(new File(outdir).getAbsolutePath() + File.separator + outfileName);
        System.out.println("Writing to to this file: " + outfile.getAbsolutePath());
        
        // DOM parsing:
        DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
        DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
        Document doc = dBuilder.parse(filename);

        // optional, but recommended
        // read this -
        // http://stackoverflow.com/questions/13786607/normalization-in-dom-parsing-with-java-how-does-it-work
        doc.getDocumentElement().normalize();

        // System.out.println("Root element :" +
        // doc.getDocumentElement().getNodeName());

        D3DMeshdata mesh = new D3DMeshdata();;
        NodeList nList = doc.getElementsByTagName("geometry");

        for (int temp = 0; temp < nList.getLength(); temp++) {

            Node nNode = nList.item(temp);

            String node_name = nNode.getAttributes().getNamedItem("name").getNodeValue();
            String node_id = nNode.getAttributes().getNamedItem("id").getNodeValue();
            mesh.geometryId = node_id;
            System.out.println("\nGeometry found :" + nNode.getNodeName() + " " + node_name + " " + node_id);

            if (nNode.getNodeType() == Node.ELEMENT_NODE) {

                Element eElement = (Element) nNode;

                input_el[] inputTypes = getEmptyInputTable();
                // first get vertices source:
                Node v = eElement.getElementsByTagName("vertices").item(0);
                Element vpos = getChildElement(eElement, "input", "semantic", "POSITION", true);
                parseInput(vpos, inputTypes);
                int cur = input_el_type.POSITION.ordinal();
                System.out.println("vertex positions are in node id " + inputTypes[cur].source);
                //System.out.println("by id: " + doc.getElementById(positions_id)); would not work as we have no schema loaded
                NodeList sourceList = eElement.getElementsByTagName("source");
                Element mesh_pos = getElement(sourceList, "source", "id", inputTypes[cur].source, true);
                Element floatArrayNode = (Element) mesh_pos.getElementsByTagName("float_array").item(0); 
                String float_array = floatArrayNode.getTextContent();
                mesh.numVerts = Integer.valueOf(floatArrayNode.getAttribute("count"));
                System.out.println(" num verts: " + mesh.numVerts);
                //System.out.println(" verts: " + float_array);
                mesh.verts = new float[mesh.numVerts];
                parse_floats(mesh.verts, float_array);
                
                // now parse faces (polylist) for vertex, normal and texcoord index
                boolean modeTriangle = false;  // signal polylist or triangle use
                Element pel = (Element) eElement.getElementsByTagName("polylist").item(0);
                // if we could not find polylist try triangles:
                if (pel == null) {
                	pel = (Element) eElement.getElementsByTagName("triangles").item(0);
                	if (pel != null) {
                		modeTriangle = true;
                	}
                }
                // if pel still null we could not parse geometry:
                if (pel == null) {
                	fail("neither polygons nor triangles found. Cannot parse.");
                }
                if (pel != null) {
	                NodeList inputList = pel.getElementsByTagName("input");
	                for (int i = 0; i < inputList.getLength(); i++) {
	                    Node node = inputList.item(i);
	                    if (node instanceof Element) {
	                        Element child = (Element) node;
	                        parseInput(child, inputTypes);
	                    }
	                }
                } 
                if (inputTypes[input_el_type.TEXCOORD.ordinal()].source == null) {
                    System.out.println("WARNING: input file does not include texture coordinates. Use UV mapping and re-export.");
                    fakeTextureMapping = true;
                }
                mesh.numFaces = Integer.parseInt(pel.getAttribute("count"));
                System.out.println("numFaces = " + mesh.numFaces);
                
                // normals
                cur = input_el_type.NORMAL.ordinal();
                Element normal_source = getElement(sourceList, "source", "id", inputTypes[cur].source, true);
                floatArrayNode = (Element) normal_source.getElementsByTagName("float_array").item(0); 
                float_array = floatArrayNode.getTextContent();
                mesh.numNormals = Integer.valueOf(floatArrayNode.getAttribute("count"));
                System.out.println(" num normals: " + mesh.numNormals);
                //System.out.println(" normals: " + float_array);
                mesh.normals = new float[mesh.numNormals];
                parse_floats(mesh.normals, float_array);
                
                // texcoord
                if (!fakeTextureMapping) {
                    cur = input_el_type.TEXCOORD.ordinal();
                    Element texcoord_source = getElement(sourceList, "source", "id", inputTypes[cur].source, true);
                    floatArrayNode = (Element) texcoord_source.getElementsByTagName("float_array").item(0); 
                    float_array = floatArrayNode.getTextContent();
                    mesh.numTexcoords = Integer.valueOf(floatArrayNode.getAttribute("count"));
                    System.out.println(" num texture coords: " + mesh.numTexcoords);
                    //System.out.println(" coords: " + float_array);
                    mesh.texcoords = new float[mesh.numTexcoords];
                    parse_floats(mesh.texcoords, float_array);
                }
                
                // indexes in polylist
                if (!modeTriangle) {
	                Element vcount_el = (Element) pel.getElementsByTagName("vcount").item(0);
	                int[] vcount = new int[mesh.numFaces];
	                parse_ints(vcount, vcount_el.getTextContent()); 
	                for (int i = 0; i < vcount.length; i++) {
	                    if (vcount[i] != 3) fail("cannot handle arbitrary polyons - only triangles allowed");
	                }
                }
                Element p_el = (Element) pel.getElementsByTagName("p").item(0);
                Integer[] p = parse_ints(p_el.getTextContent()); 
                int groupSize = p.length / mesh.numFaces / 3; // inputs in <p> are grouped together in this size

                // index buffer for triangles
                mesh.indexBuffer = new ArrayList<Integer>();
                int vertexOffset = inputTypes[input_el_type.VERTEX.ordinal()].offset;
                for (int i = 0; i < p.length; i += groupSize) {
                    mesh.indexBuffer.add(p[i + vertexOffset]);
                }
                if (!fakeTextureMapping) {
                    // texture coordinate index buffer for vertices
                    mesh.texIndexBuffer = new ArrayList<Integer>();
                    int textureOffset = inputTypes[input_el_type.TEXCOORD.ordinal()].offset;
                    for (int i = 0; i < p.length; i += groupSize) {
                        mesh.texIndexBuffer.add(p[i + textureOffset]);
                    }
                }
                // normal index buffer for vertices
                mesh.normalIndexBuffer = new ArrayList<Integer>();
                int normalOffset = inputTypes[input_el_type.NORMAL.ordinal()].offset;
                for (int i = 0; i < p.length; i += groupSize) {
                    mesh.normalIndexBuffer.add(p[i + normalOffset]);
                }
                
                mesh.meshName = node_name;
                List<Animation> anis = parseAnimations(node_name, doc, false);
                if (!anis.isEmpty()) {
                    anis = orderAndConvert(anis);
                }
                
                List<SkinnedAnimation> skinnedAnis = parseSkinnedAnimations(node_name, doc, mesh);
                if (!skinnedAnis.isEmpty()) {
                    finishBonesSetup(boneList, skinnedAnis, doc);
                }
                
                // post processing collada data:
                createD3DVertexList(mesh, skinnedAnis);
                
                // write binary file:
                //ObjectOutputStream oos = new ObjectOutputStream(new BufferedOutputStream(new FileOutputStream(outfile)));
                DataOutputStream oos = new DataOutputStream(new BufferedOutputStream(new FileOutputStream(outfile)));
                List<D3DVertex> all = mesh.d3dVertexList;
                boolean isSkinned = false;
                // write type: 0 means not skinned, 1 means skinned
                if (all.size() > 0 && all.get(0).weight != null) {
                    writeInt(oos, 1);
                    isSkinned = true;
                } else {
                    writeInt(oos, 0);
                }
                if (isSkinned) {
                    // for skinned animations we begin with joints list
                    assert !skinnedAnis.isEmpty();
                    writeInt(oos, skinnedAnis.size());
                    for (int i = 0; i < skinnedAnis.size(); i++) {
                        SkinnedAnimation a = skinnedAnis.get(i);
                        writeInt(oos, a.name.length());
                        writeString(oos, a.name);
                        // Joints/Bones
                        assert a.joints.size() == a.bones.size();
                        writeInt(oos, a.joints.size());
                        for (int j = 0; j < a.joints.size(); j++) {
                            Bone bone = a.bones.get(j);
                            Joint joint = a.joints.get(j);
                            writeInt(oos, bone.parentId);
                            writeMatrix(oos, joint.invBindMatrix);
                            writeMatrix(oos, bone.bindPose);
                            int numKey = 0;
                            List<BezTriple> curves = null;
                            if (bone.transformations.size() > 0) {
                                curves = bone.transformations.get(0).beziers;
                                numKey = curves.size();
                            }
                            writeInt(oos, numKey);  // number of keyframes
                            for (int c = 0; c < numKey; c++) {
                                writeFloat(oos, curves.get(c).cp[0] * FPS);
                                writeMatrix(oos, curves.get(c).keyFrameMatrix);
                            }
                        }
                        // now write transformations for this clip
                        // #number of keyframes
                        //writeInt(oos, a.bones.get(0).transformations.size());
                        
                    }
                }
                assert (all.size() % 3 == 0); // assert we have triples
                // fix triangle order:
                if (fixaxis) {
                    for (int i = 0; i < all.size(); i+=3) {
                    	D3DVertex s = all.get(i+1);
                    	all.set(i+1, all.get(i+2));
                    	all.set(i+2, s);
                    }
                }
                // vertices length + data
                writeInt(oos, all.size()*3);
                for (int i = 0; i < all.size(); i++) {
                	if (fixaxis) {
	                    writeFloat(oos, all.get(i).x);
	                    writeFloat(oos, all.get(i).z);
	                    writeFloat(oos, all.get(i).y);
                	} else {
                        writeFloat(oos, all.get(i).x);
                        writeFloat(oos, all.get(i).y);
                        writeFloat(oos, all.get(i).z);
                	}
                }
                for (int i = 0; i < all.size(); i++) {
                    writeFloat(oos, all.get(i).cu);
                    writeFloat(oos, 1.0f - all.get(i).cv);
                }
                // write normals
                for (int i = 0; i < all.size(); i++) {
                	if (fixaxis) {
	                    writeFloat(oos, all.get(i).nx);
	                    writeFloat(oos, all.get(i).nz);
	                    writeFloat(oos, all.get(i).ny);
                	} else {
	                    writeFloat(oos, all.get(i).nx);
	                    writeFloat(oos, all.get(i).ny);
	                    writeFloat(oos, all.get(i).nz);
                	}
                }
                if (isSkinned) {
                    for (int i = 0; i < all.size(); i++) {
                        int bonePack = pack(all.get(i).weight);
                        writeInt(oos, bonePack);
                    }
                    for (int i = 0; i < all.size(); i++) {
                        writeFloat(oos, all.get(i).weight[0].weight);
                        writeFloat(oos, all.get(i).weight[1].weight);
                        writeFloat(oos, all.get(i).weight[2].weight);
                        writeFloat(oos, all.get(i).weight[3].weight);
                    }
                }
                // vert index length + data
                writeInt(oos, all.size());
                for (int i = 0; i < mesh.indexBuffer.size(); i++) {
                    writeInt(oos, i);
                }
                // animations (movement, no clips)
                if (anis.size() == 0) {
                    // write 0 to indicate that no animation is present
                    writeInt(oos, 0);
                } else {
                    String ani_name = node_name;
                    writeInt(oos, ani_name.length());
                    writeString(oos, ani_name);
                    // write bezTriple data
                    assert anis.size() == 9;
                    for (int i = 0; i < 9; i++) {
                        Animation a = anis.get(i);
                        writeInt(oos, a.beziers.size());
                        for (int j = 0; j < a.beziers.size(); j++) {
                            BezTriple b = a.beziers.get(j);
                            writeFloat(oos, b.h1[0]);
                            writeFloat(oos, b.h1[1]);
                            writeFloat(oos, b.cp[0]);
                            writeFloat(oos, b.cp[1]);
                            writeFloat(oos, b.h2[0]);
                            writeFloat(oos, b.h2[1]);
                        }
                    }
                }
                
                // write number of available animations
                //oos.writeFloat(pi); // uses big endian
                oos.close();
                //File out = new File(outdir + File.separator + )
                
                
                // System.out.println("mesh : " + eElement.getAttribute("id"));
                //System.out.println("mesh : " + eElement.getElementsByTagName("mesh").item(0));
                //System.out.println("source : " + eElement.getElementsByTagName("source").item(0).getAttributes().getNamedItem("id"));
                // System.out.println("Nick Name : " +
                // eElement.getElementsByTagName("nickname").item(0).getTextContent());
                // System.out.println("Salary : " +
                // eElement.getElementsByTagName("salary").item(0).getTextContent());
                fakeTextureMapping = false;
            }
        }

    }

    private void finishBonesSetup(List<Bone> bones, List<SkinnedAnimation> skinnedAnis, Document doc) {
        assert skinnedAnis.size() == 1;
        // currently expect only one animation:
        for (Bone bone : bones) {
            List<Animation> alist = parseAnimations(bone.name, doc, true);
            bone.transformations = alist;
        }
        skinnedAnis.get(0).bones = bones;
    }

    private int pack(BoneWeight[] weight) {
        int pack = 0;
        for (int i = 3; i >= 0; i--) {
            pack <<= 8;
            pack |= (weight[i].joint) & 0xff;
        }
        return pack;
    }

    private List<SkinnedAnimation> parseSkinnedAnimations(String mesh_name, Document doc, D3DMeshdata mesh) {
        List<SkinnedAnimation> anis = new ArrayList<SkinnedAnimation>();
        System.out.println("Skinned Animations for " + mesh_name + ":");
        Element vis_scenes = (Element)doc.getElementsByTagName("library_visual_scenes").item(0);
        Element meshNode = (Element) getChildElement(vis_scenes, "node", "id", mesh_name, false);
        Element inst_controller = (Element) vis_scenes.getElementsByTagName("instance_controller").item(0);
        if (inst_controller == null) {
            // no skinned animations available
            return anis;
        }
        String instControllerName = inst_controller.getAttribute("url").substring(1);
        Element skeleton = (Element) inst_controller.getElementsByTagName("skeleton").item(0);
        String skeletonName = skeleton.getTextContent().substring(1);  
        System.out.println("found controller: " + instControllerName +  " skeleton: " + skeletonName);
        // skeletonName is root bone, instControllerName begins with node name from visual_scenes
        // get node with bones:
        Element bonesNode = null;
        Element scene = (Element) vis_scenes.getElementsByTagName("visual_scene").item(0);
        for (int temp = 0; temp < scene.getChildNodes().getLength(); temp++) {
            Node child = scene.getChildNodes().item(temp);
            if (child.getNodeType() != Node.ELEMENT_NODE) continue;
            Element child_el = (Element) child;
            //System.out.println(" vis_scene child: " + child_el.getNodeName());
            String node_name = child.getAttributes().getNamedItem("id").getTextContent();
            if (instControllerName.startsWith(node_name)) {
                System.out.println("Bone info here: " + node_name);
                bonesNode = child_el;
            }
        }
        
        NodeList possibleRootBones = bonesNode.getChildNodes();//bonesNode.getElementsByTagName("node");
        for (int temp = 0; temp < possibleRootBones.getLength(); temp++) {
            if (possibleRootBones.item(temp).getNodeType() != Node.ELEMENT_NODE) {
                continue;
            }
            Element rootBone = (Element) possibleRootBones.item(temp);
            if (rootBone.getNodeName() != "node") {
                continue;
            }
            System.out.println("Root Bone " + temp + " " + rootBone.getAttribute("id"));
            //Element rootBone = getChildElement(bonesNode, "node", "id", skeletonName, true);
            parseBoneHierarchy(rootBone, boneList, -1);
            assertBoneHierarchy(boneList);
        }        
        
        Element controller = (Element)doc.getElementsByTagName("library_controllers").item(0);
        Element skin = (Element) getChildElement(controller, "skin", "source", "#" + mesh.geometryId, false);
        Element clipController = (Element)skin.getParentNode();
        String animationClipName = clipController.getAttribute("name");
        if (animationClipName == null) {
            animationClipName = clipController.getAttribute("id");
        }
        Element bind_shape_matrix = (Element) skin.getElementsByTagName("bind_shape_matrix").item(0);
        SkinnedAnimation a = new SkinnedAnimation();
        anis.add(a);
        a.meshName = mesh_name;
        a.name = animationClipName;
        System.out.println("Animation Clip: " + a.name);
        a.bindPoseMatrix = parse_floats(bind_shape_matrix.getTextContent());
        Element joints = (Element) skin.getElementsByTagName("joints").item(0);
        Element joints_semantic = getChildElement(joints, "input", "semantic", "JOINT", true);
        String jointSource = joints_semantic.getAttribute("source").substring(1);
        Element inv_bind_semantic = getChildElement(joints, "input", "semantic", "INV_BIND_MATRIX", true);
        String invBindSource = inv_bind_semantic.getAttribute("source").substring(1);

        Element vertex_weights_el = (Element) skin.getElementsByTagName("vertex_weights").item(0);
        Element weight_el = getChildElement(vertex_weights_el, "input", "semantic", "WEIGHT", true);
        String weightSource = weight_el.getAttribute("source").substring(1);
        
        // now gather data for all joints:
        Element joint_names_el = getChildElement(skin, "source", "id", jointSource, true);
        String flatJointNames = joint_names_el.getElementsByTagName("Name_array").item(0).getTextContent();
        String[] jointNames = parse_strings(flatJointNames);
        for (int i = 0; i < jointNames.length; i++) {
            Joint j = new Joint();
            a.joints.add(j);
            j.name = jointNames[i];
            assert a.joints.get(i).name.equals(boneList.get(i).name); // if this differs we need mapping
        }
        Element inv_bind_matrix_source_el = getChildElement(skin, "source", "id", invBindSource, true);
        String flatMatrices = inv_bind_matrix_source_el.getElementsByTagName("float_array").item(0).getTextContent();
        Float[] invBindMatrices = parse_floats(flatMatrices);
        assert invBindMatrices.length == a.joints.size() * 16;
        for (int i = 0; i < a.joints.size(); i++) {
            Joint j = a.joints.get(i);
            j.invBindMatrix = Arrays.copyOfRange(invBindMatrices, i*16, (i+1)*16);
        }
        
        Element wight_source_el = getChildElement(skin, "source", "id", weightSource, true);
        String weightValues = wight_source_el.getElementsByTagName("float_array").item(0).getTextContent();
        Float[] weights = parse_floats(weightValues);
        Integer[] vcount = parse_ints(vertex_weights_el.getElementsByTagName("vcount").item(0).getTextContent());
        Integer[] v = parse_ints(vertex_weights_el.getElementsByTagName("v").item(0).getTextContent());
        int vpos = 0;
        for (int i = 0; i < vcount.length; i++) {
            if (!discardBoneInfluenceGreater4)
                assert vcount[i] <= 4;  // we only handle up to 4 bone influences
            BoneWeight[] w = new BoneWeight[4];
            for (int j = 0; j < 4; j++) {
                w[j] = new BoneWeight();
                w[j].joint = -1;  // indicate unused bone
                w[j].weight = 0.0f;
            }
            for (int j = 0; j < vcount[i]; j++) {
                int bone = v[vpos++];
                assert bone >= 0;  // we only handle real joints, not the -1 == bind shape syntax
                float weight = weights[v[vpos++]];
                if (j < 4) {
                    w[j].joint = bone; 
                    w[j].weight = weight;
                }
            }
            a.weights.add(w);
        }
        //assert weights.length == a.joints.size() * 16;
        //Element meshNode = (Element) getChildElement(vis_scenes, "node", "id", mesh_name, false);
//        if (nList.getLength() == 0) return anis;
//        Node lib = nList.item(0);
//        for (int temp = 0; temp < lib.getChildNodes().getLength(); temp++) {
//            // animations
//            Node nNode = lib.getChildNodes().item(temp);
//            if (nNode.getNodeType() == Node.ELEMENT_NODE) {
//            }
//        }
        return anis;
    }
    
    private void assertBoneHierarchy(List<Bone> bones) {
        // assert correct hierarchy (parent bones have to come before child bones)
        assert bones.get(0).parentId == -1;  // root is a t index 0
        for (int i = 1; i < bones.size(); i++) {
            assert bones.get(i).parentId < i;
        }
    }

    private void parseBoneHierarchy(Element bone_el, List<Bone> bones, int parentId) {
        // recursively parse bone hierarchy
        Bone bone = new Bone();
        bone.id = bones.size();
        bone.parentId = parentId;
        bone.name = bone_el.getAttribute("id");
        Element matrix = (Element)bone_el.getElementsByTagName("matrix").item(0);
        bone.bindPose = parse_floats(matrix.getTextContent());
        bones.add(bone);
//      NodeList children = bone_el.getElementsByTagName("node");
//      for (int i = 0; i < children.getLength(); i++) {
//          parseBoneHierarchy((Element)children.item(i), bones, bone.id);
//      }
      NodeList children = bone_el.getChildNodes();
      for (int i = 0; i < children.getLength(); i++) {
          Node possibleChild = children.item(i);
          if (possibleChild.getNodeName().equals("node"))
              parseBoneHierarchy((Element)possibleChild, bones, bone.id);
      }
    }

    private class BoneWeight {
        public int joint; // bone/join index from joint list
        public float weight;
        public String toString() {
            return "" + joint + ":" + weight + "";
        }
        @Override
        public int hashCode() {
            final int prime = 31;
            int result = 1;
            result = prime * result + getOuterType().hashCode();
            result = prime * result + joint;
            result = prime * result + Float.floatToIntBits(weight);
            return result;
        }
        @Override
        public boolean equals(Object obj) {
            if (this == obj)
                return true;
            if (obj == null)
                return false;
            if (getClass() != obj.getClass())
                return false;
            BoneWeight other = (BoneWeight) obj;
            if (!getOuterType().equals(other.getOuterType()))
                return false;
            if (joint != other.joint)
                return false;
            if (Float.floatToIntBits(weight) != Float.floatToIntBits(other.weight))
                return false;
            return true;
        }
        private ColladaImport getOuterType() {
            return ColladaImport.this;
        }
    }
    
    private List<Animation> orderAndConvert(List<Animation> anis) {
        // make sure we have an order of pos, rotation, scale with each having x,y,z animation pathes
        List<Animation> anisOrdered = new ArrayList<Animation>(9);
        // position: change time to frame number (*FPS) 
        Animation a = getAnimationChannel(anis, "location.X");
        for (BezTriple bez : a.beziers) {
            bez.cp[0] *= FPS;
        }
        anisOrdered.add(a);
        a = getAnimationChannel(anis, "location.Y");
        for (BezTriple bez : a.beziers) {
            bez.cp[0] *= FPS;
        }
        anisOrdered.add(a);
        a = getAnimationChannel(anis, "location.Z");
        for (BezTriple bez : a.beziers) {
            bez.cp[0] *= FPS;
        }
        anisOrdered.add(a);
        
        // rotation: change time to fame number and degree to radians
        a = getAnimationChannel(anis, "rotationX.ANGLE");
        for (BezTriple bez : a.beziers) {
            bez.cp[0] *= FPS;
            bez.cp[1] = (float) Math.toRadians(bez.cp[1]);
        }
        anisOrdered.add(a);
        a = getAnimationChannel(anis, "rotationY.ANGLE");
        for (BezTriple bez : a.beziers) {
            bez.cp[0] *= FPS;
            bez.cp[1] = (float) Math.toRadians(bez.cp[1]);
        }
        anisOrdered.add(a);
        a = getAnimationChannel(anis, "rotationZ.ANGLE");
        for (BezTriple bez : a.beziers) {
            bez.cp[0] *= FPS;
            bez.cp[1] = (float) Math.toRadians(bez.cp[1]);
        }
        anisOrdered.add(a);
        anisOrdered.add(getAnimationChannel(anis, "scale.X"));
        anisOrdered.add(getAnimationChannel(anis, "scale.Y"));
        anisOrdered.add(getAnimationChannel(anis, "scale.Z"));
        return anisOrdered;
    }

    private Animation getAnimationChannel(List<Animation> anis, String channel) {
        for (Animation animation : anis) {
            if (animation.target.endsWith(channel)) return animation;
        }
        fail("unknown animation channel found: " + channel);
        return null;
    }

    enum AnimationSampler {INPUT, OUTPUT, INTERPOLATION, IN_TANGENT, OUT_TANGENT};
    
    private class BezTriple {
        float h1[] = new float[2];
        float cp[] = new float[2];
        float h2[] = new float[2];
        public Float[] keyFrameMatrix;
    }
    
    private class Bone {
        public List<Animation> transformations;
        Float[] bindPose;
        int id;
        int parentId;
        String name;
    }

    private List<Bone> boneList = new ArrayList<Bone>();

    private List<Animation> parseAnimations(String mesh, Document doc, boolean isMatrixMode) {
        List<Animation> anis = new ArrayList<Animation>();
        System.out.println("Animations for " + mesh + ":");
        NodeList nList = doc.getElementsByTagName("library_animations");
        if (nList.getLength() == 0) return anis;
        Node lib = nList.item(0);
        for (int temp = 0; temp < lib.getChildNodes().getLength(); temp++) {
            // animations
            Node nNode = lib.getChildNodes().item(temp);
            if (nNode.getNodeType() == Node.ELEMENT_NODE) {
                Element ani = (Element) nNode;
                assert ani.getTagName().equals("animation");
                NodeList sub = ani.getChildNodes();
                for (int i = 0; i < sub.getLength(); i++) {
                    Node n = sub.item(i);
                    if (n.getNodeType() != Node.ELEMENT_NODE)
                        continue;
                    // source, sampler and channel
                    Element s = (Element) n;
                    if (s.getTagName().equals("channel")) {
                        String target = s.getAttribute("target");
                        // we are only interested in animations of our mesh
                        if (!target.startsWith(mesh+"/")) continue;
                        System.out.println("found channel " + target);
                        Animation a = new Animation();
                        anis.add(a);
                        a.target = target;
                        a.source = s.getAttribute("source").substring(1);
                        a.beziers = new ArrayList<BezTriple>();
                        // dive into sampler:
                        Element sampler = getElement(sub, "sampler", "id", a.source, true);
                        NodeList samplerChildNodes = sampler.getChildNodes();
                        for ( int j = 0; j < samplerChildNodes.getLength(); j++) {
                            Node ns = samplerChildNodes.item(j);
                            if (ns.getNodeType() != Node.ELEMENT_NODE) continue;
                            String semantic = ((Element)ns).getAttribute("semantic");
                            String source = ((Element)ns).getAttribute("source").substring(1);
                            Element e;
                            Float floats[];
                            switch(AnimationSampler.valueOf(semantic)) {
                            case INPUT:
                                e = getElement(sub, "source", "id", source, true);
                                floats = parse_floats(e.getElementsByTagName("float_array").item(0).getTextContent());
                                for (int c = 0; c < floats.length; c++) {
                                    BezTriple b = getCreateBezier(a.beziers, c);
                                    b.cp[0] = floats[c];
                                }
                                break;
                            case OUTPUT:
                                e = getElement(sub, "source", "id", source, true);
                                floats = parse_floats(e.getElementsByTagName("float_array").item(0).getTextContent());
                                if (!isMatrixMode) {
                                    for (int c = 0; c < floats.length; c++) {
                                        BezTriple b = getCreateBezier(a.beziers, c);
                                        b.cp[1] = floats[c];
                                    }
                                } else {
                                    int numKeyframes = floats.length / 16; 
                                    assert numKeyframes == a.beziers.size();
                                    int pos = 0;
                                    for (int keyFrame = 0; keyFrame < numKeyframes; keyFrame++) {
                                        BezTriple b = getCreateBezier(a.beziers, keyFrame);
                                        b.keyFrameMatrix = new Float[16];
                                        for (int c = 0; c < 16; c++) {
                                            b.keyFrameMatrix[c] = floats[pos++];
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
        return anis;
    }

    private BezTriple getCreateBezier(List<BezTriple> beziers, int i) {
        if (i >= 0 && i < beziers.size()) return beziers.get(i);
        // this index is not available, add elements until it is:
        while (beziers.size() <= i)
            beziers.add(new BezTriple());
        return beziers.get(i);
    }

    private void createD3DVertexList(D3DMeshdata mesh, List<SkinnedAnimation> animations) {
        Random rand = new Random();
        int numVertices = mesh.indexBuffer.size();
        SkinnedAnimation ani = findAnimation(mesh.meshName, animations);
        boolean isAnimated = ani != null;
        assert mesh.indexBuffer.size() / 3 == mesh.numFaces;
        if (!fakeTextureMapping)
            assert mesh.indexBuffer.size() == mesh.texIndexBuffer.size();
        assert mesh.verts.length / 3 <= numVertices;
        assert mesh.normals.length / 3 <= numVertices;
        if (!fakeTextureMapping)
            assert mesh.texcoords.length / 2 == numVertices;
        //int numDistinctVerts = mesh.numFaces * 3;
        mesh.d3dVertexList = new ArrayList<D3DVertex>(numVertices);  // indexes describe one vertex
        // go through indexes and create d3d vertexes on the fly (no optimization)
        for (int i = 0; i < mesh.indexBuffer.size(); i++) {
            // create full d3d vertex data:
            D3DVertex v = new D3DVertex();
            int vertArrayPos = mesh.indexBuffer.get(i) * 3;
            v.x = mesh.verts[vertArrayPos];
            v.y = mesh.verts[vertArrayPos+1];
            v.z = mesh.verts[vertArrayPos+2];
            if (fakeTextureMapping) {
                v.cu = rand.nextFloat();
                v.cv = rand.nextFloat();
            } else {
                int textureArrayPos = mesh.texIndexBuffer.get(i) * 2;
                v.cu = mesh.texcoords[textureArrayPos];
                v.cv = mesh.texcoords[textureArrayPos+1];
            }
            int normalArrayPos = mesh.normalIndexBuffer.get(i) * 3;
            v.nx = mesh.normals[normalArrayPos];
            v.ny = mesh.normals[normalArrayPos+1];
            v.nz = mesh.normals[normalArrayPos+2];
            if (isAnimated) {
                v.weight = ani.weights.get(vertArrayPos/3);
                //System.out.print(" " + (mesh.indexBuffer.get(i)));
            }
            mesh.d3dVertexList.add(v);
        }
        // calculate waste (== useless vertices, because of duplicates)
        Set<D3DVertex> set = new HashSet<D3DVertex>();
        for (D3DVertex v : mesh.d3dVertexList) {
            //v.nx = 0;
            //v.ny = 0;
            //v.nz = 0;
            //v.cu = 0;
            //v.cv = 0;
            set.add(v);
        }
        int addVert = numVertices - (mesh.verts.length / 3);
        int percentage = addVert * 100 /  (mesh.verts.length / 3);
        System.out.println("collada created " + addVert + " additional vertices (" + percentage + " %)" );
        System.out.println("Wasted: " + (mesh.d3dVertexList.size() - set.size()));
    }

    private SkinnedAnimation findAnimation(String meshName, List<SkinnedAnimation> animations) {
        for (SkinnedAnimation skinnedAnimation : animations) {
            if (skinnedAnimation.meshName.equals(meshName))
                    return skinnedAnimation;
        }
        return null;
    }

    private void parseInput(Element input, input_el[] inputTypes) {
        String semantic = input.getAttribute("semantic");
        String source = input.getAttribute("source");
        String offset = input.getAttribute("offset");
        input_el_type cur;
        try {
            cur = input_el_type.valueOf(semantic);
        } catch (IllegalArgumentException e) {
            // we don't parse unknown input types - just return
            return;
        }
        if (source != null && source.length() > 0) {
            if (source.startsWith("#"))
                source = source.substring(1);
            inputTypes[cur.ordinal()].source = source; 
        }
        if (offset != null && offset.length() > 0) {
            int intOffset = Integer.valueOf(offset);
            inputTypes[cur.ordinal()].offset = intOffset; 
        }
    }

    private void writeMatrix(DataOutputStream oos, Float[] matrix) throws IOException {
        for (int i = 0; i < 16; i++) {
            writeFloat(oos, matrix[i]);
        }
    }

    private void writeString(DataOutputStream oos, String s) throws IOException {
        // don't pay attention to unicode, just write the char bytes
        // will only work for clean ASCII values < 128
        for (int i = 0; i < s.length(); i++) {
            byte b = (byte)s.charAt(i);
            oos.writeByte(b);
        }
    }

    private void writeInt(DataOutputStream oos, int i) throws IOException {
        oos.writeInt(Integer.reverseBytes(i));
    }

    private void writeFloat(DataOutputStream oos, float f) throws IOException {
        int fi = Float.floatToIntBits(f);
        oos.writeInt(Integer.reverseBytes(fi));
    }

    private void parse_floats(float[] verts, String float_string) {
        StringTokenizer tok = new StringTokenizer(float_string);
        int pos = 0;
        while (tok.hasMoreTokens()) {
            String fs = tok.nextToken();
            //System.out.println(Float.parseFloat(fs));
            verts[pos++] = Float.parseFloat(fs);
        }
    }

    private String[] parse_strings(String s) {
        StringTokenizer tok = new StringTokenizer(s);
        List<String> strings = new ArrayList<String>();
        while (tok.hasMoreTokens()) {
            strings.add(tok.nextToken());
        }
        return strings.toArray(new String[0]);
    }

    private Float[] parse_floats(String float_string) {
        StringTokenizer tok = new StringTokenizer(float_string);
        List<Float> floats = new ArrayList<Float>();
        while (tok.hasMoreTokens()) {
            String fs = tok.nextToken();
            floats.add(Float.parseFloat(fs));
        }
        return floats.toArray(new Float[0]);
    }

    private void parse_ints(int[] vals, String int_string) {
        StringTokenizer tok = new StringTokenizer(int_string);
        int pos = 0;
        while (tok.hasMoreTokens()) {
            String fs = tok.nextToken();
            //System.out.println(Float.parseFloat(fs));
            vals[pos++] = Integer.parseInt(fs);
        }
    }

    private Integer[] parse_ints(String int_string) {
        StringTokenizer tok = new StringTokenizer(int_string);
        List<Integer> ints = new ArrayList<Integer>();
        while (tok.hasMoreTokens()) {
            String fs = tok.nextToken();
            ints.add(Integer.parseInt(fs));
        }
        return ints.toArray(new Integer[0]);
    }

    private Element getElement(NodeList list, String tagname, String attname, String attvalue, boolean exitIfNotFound) {
        for (int i = 0; i < list.getLength(); i++) {
            Node node = list.item(i);
            if (node instanceof Element) {
                Element child = (Element) node;
                if (child.getAttribute(attname).equals(attvalue))
                    return child;
            }
        }
        if (exitIfNotFound)
            fail("tag <" + tagname + "> missing mandatory attribute: "+ attname + "=\"" + attvalue + "\"");
        //fail("Attribute " + attname + " = " + attvalue + " not found in tag <" + tagname + ">");
        return null;
    }

    private Element getChildElement(Element parent, String tagname, String attname, String attvalue, boolean exitIfNotFound) {
        NodeList list = parent.getElementsByTagName(tagname);
        return getElement(list, tagname, attname, attvalue, exitIfNotFound);
    }

    static private void failUsage(String string) {
        System.out.println(string);
        usage();
        System.exit(0);
        
    }

    static private void fail(String string) {
        System.out.println(string);
        System.exit(0);
        
    }

    static void usage() {
        System.out.println("usage: java de.fehrprice.collada.ColladaImport [-outdir <output_directory] [-fixaxis] collada_file");
        System.out.println("     Use Apply for bones and mesh objects in blender before exporting!");
    }

    input_el[] getEmptyInputTable() {
        int len = input_el_type.values().length;
        input_el[] in = new input_el[len];
        for (int i = 0; i < len; i++) {
            in[i] = new input_el();
            in[i].type = input_el_type.values()[i];
            in[i].offset = 0;
            in[i].source = null;
        }
        return in;
    }
    // local data structures
    enum input_el_type { POSITION, VERTEX, NORMAL, TEXCOORD};
    private class input_el {
        input_el_type type;
        int offset;
        String source;
    }
}
