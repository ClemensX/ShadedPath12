/*
 * ColladaImport - read collada file and output custom binary format .b
 * 
 * (c) copyright 2013 Clemens Fehr - All rights reserved
 * 
 *  run with -ea (enable assertions) as many collada structure checks are simple assertions
 */
package de.fehrprice.collada;

import java.io.BufferedOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
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

//import de.fehrprice.collada.ColladaImport.Animation;
//import de.fehrprice.collada.ColladaImport.BezTriple;
//import de.fehrprice.collada.ColladaImport.Bone;
//import de.fehrprice.collada.ColladaImport.D3DVertex;
//import de.fehrprice.collada.ColladaImport.Joint;
//import de.fehrprice.collada.ColladaImport.SkinnedAnimation;
import de.fehrprice.collada.ColladaImport.input_el_type;

import java.util.Arrays;
import java.util.Collections;

public class ColladaImport2 {

    public static int FPS = 24;  // we want to have frame numbers for D3D, as this setting is not exported via collada we use global const
    public static String filename;
    public static String outdir = "";
    public static boolean UV_DuplicateVertex = true;  // enable vertex duplication if needed for multiple UV coords on one vertex
    public static boolean fakeTextureMapping = false;  // set to true if no UV mapping available to fake texture coord

    public static boolean discardBoneInfluenceGreater4 = true;  // bone influence > 4 will be discarded instead of failed assertion
	private static boolean fixaxis = false;

    private class D3DVertex {
        // position
        float x,y,z,a;
        // normal
        float nx, ny, nz;
        // texcoord
        float cu, cv;
        BoneWeight[] weight = null;

        
        private ColladaImport2 getOuterType() {
            return ColladaImport2.this;
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
    
    private class Geometry {
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
    
    private class Controller {
    	String name;
    }
    
    private class RenderObject {
    	String boneHierarchyId;
    	String name;
    }
    
    // parse all needed data from collada file into this
    // mixture of dom elemnts and parsed data
    private class Collada {
		public String sceneName;
		public Map<String,List<Bone>> boneRootsMap = new HashMap<>();
	    // store node tree
	    private List<ColladaNode> colladaNodeList = new ArrayList<>();
	    // direct access to nodes via id 
	    private Map<String,ColladaNode> nodeMap = new HashMap<>();
	    // store controllers
	    private Map<String,Controller> controllerMap = new HashMap<>();
	    // store id of renderable objects - details are in the other lists
	    private List<String> renderObjects = new ArrayList<>();
	    private Map<String,RenderObject> renderObjectMap = new HashMap<>();
	    private Map<String,Geometry> geometryMap = new HashMap<>();
    }
    
    private Collada collada;

    /**
     * @param args
     */
    public static void main(String[] args) {
        ColladaImport2 ci = new ColladaImport2();
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

        collada = new Collada();
        
        // optional, but recommended
        // read this -
        // http://stackoverflow.com/questions/13786607/normalization-in-dom-parsing-with-java-how-does-it-work
        Element docEl = doc.getDocumentElement();
        docEl.normalize();
    	assert(docEl.getTagName().equalsIgnoreCase("COLLADA"));
    	Element el = getSingleAssertedChildElement(docEl, "library_geometries");
    	loadGeometry(el);
    	el = getSingleAssertedChildElement(docEl, "library_controllers");
    	loadControllers(el);
    	el = getSingleAssertedChildElement(docEl, "scene");
    	el = getSingleAssertedChildElement(el, "instance_visual_scene");
    	String ivsURL = getInternalURL(el);
    	loadVisualScene(ivsURL, docEl);
    	
    	writeOutputFile(outfile);
    }
    
    // library_geometries
    private void loadGeometry(Element el) {
    	List<Element> le = getChildElements(el);
    	// load all meshes:
    	for (Element e : le) {
    		// library_geometries / geometry
    		assert("geometry".equals(e.getTagName()));
    		Geometry g = new Geometry();
    		g.geometryId = e.getAttribute("id");
    		g.meshName = e.getAttribute("name");
    		log(" load geometry: " + g.meshName);
    		// library_geometries / geometry / mesh
    		Element meshNode = getSingleAssertedChildElement(e, "mesh");
    		// determine position source
    		// library_geometries / geometry / mesh / vertices
    		Element v = getSingleAssertedChildElement(meshNode, "vertices");
    		Element posInput = getChildElement(v, "input", "semantic", "POSITION", true);
    		// library_geometries / geometry / mesh / source
    		String source = getInternalAttribute(posInput, "source");
    		Element positionSource = getChildElement(meshNode, "source", "id", source, true);
    		log(" found geometry positions: " + positionSource.getAttribute("id"));

    		// load vertices
    		// library_geometries / geometry / mesh / source / float_array
    		Element floatArrayNode = getSingleAssertedChildElement(positionSource, "float_array");//(Element) mesh_pos.getElementsByTagName("float_array").item(0); 
            String float_array = floatArrayNode.getTextContent();
            g.numVerts = Integer.valueOf(floatArrayNode.getAttribute("count"));
            log(" num verts: " + g.numVerts);
            //System.out.println(" verts: " + float_array);
            g.verts = new float[g.numVerts];
            parse_floats(g.verts, float_array);
            
            // now parse faces (polylist) for vertex, normal and texcoord index
            boolean modeTriangle = false;  // signal polylist or triangle use
    		// library_geometries / geometry / mesh / polylist
            Element pel = getSingleAssertedChildElement(meshNode, "polylist");
            // if we could not find polylist try triangles:
            if (pel == null) {
            	// library_geometries / geometry / mesh / triangles
            	pel = getSingleAssertedChildElement(meshNode, "triangles");
            	if (pel != null) {
            		modeTriangle = true;
            	}
            }
            // if pel still null we could not parse geometry:
            if (pel == null) {
            	fail("neither polygons nor triangles found. Cannot parse.");
            }
            Map<String,input_el> inputMap = new HashMap<>();
            if (pel != null) {
                NodeList inputList = pel.getElementsByTagName("input");
                for (int i = 0; i < inputList.getLength(); i++) {
                    Node node = inputList.item(i);
                    if (node instanceof Element) {
                        Element child = (Element) node;
                        parseInput(child, inputMap);
                    }
                }
            } 
            if (inputMap.get("TEXCOORD").source == null) {
                System.out.println("WARNING: input file does not include texture coordinates. Use UV mapping and re-export.");
                fakeTextureMapping = true;
            }
            g.numFaces = Integer.parseInt(pel.getAttribute("count"));
            System.out.println("numFaces = " + g.numFaces);
           
            // normals
            input_el cur;
            cur = inputMap.get("NORMAL");
            Element normal_source = getChildElement(meshNode, "source", "id", cur.source, true);
            floatArrayNode = (Element) normal_source.getElementsByTagName("float_array").item(0); 
            float_array = floatArrayNode.getTextContent();
            g.numNormals = Integer.valueOf(floatArrayNode.getAttribute("count"));
            System.out.println(" num normals: " + g.numNormals);
            //System.out.println(" normals: " + float_array);
            g.normals = new float[g.numNormals];
            parse_floats(g.normals, float_array);
            
            // texcoord
            if (!fakeTextureMapping) {
                cur = inputMap.get("TEXCOORD");
                Element texcoord_source = getChildElement(meshNode, "source", "id", cur.source, true);
                floatArrayNode = (Element) texcoord_source.getElementsByTagName("float_array").item(0); 
                float_array = floatArrayNode.getTextContent();
                g.numTexcoords = Integer.valueOf(floatArrayNode.getAttribute("count"));
                System.out.println(" num texture coords: " + g.numTexcoords);
                //System.out.println(" coords: " + float_array);
                g.texcoords = new float[g.numTexcoords];
                parse_floats(g.texcoords, float_array);
            }

            // indexes in polylist
            if (!modeTriangle) {
                Element vcount_el = (Element) pel.getElementsByTagName("vcount").item(0);
                int[] vcount = new int[g.numFaces];
                parse_ints(vcount, vcount_el.getTextContent()); 
                for (int i = 0; i < vcount.length; i++) {
                    if (vcount[i] != 3) fail("cannot handle arbitrary polyons - only triangles allowed");
                }
            }
            Element p_el = (Element) pel.getElementsByTagName("p").item(0);
            Integer[] p = parse_ints(p_el.getTextContent()); 
            int groupSize = p.length / g.numFaces / 3; // inputs in <p> are grouped together in this size

            // index buffer for triangles
            g.indexBuffer = new ArrayList<Integer>();
            int vertexOffset = inputMap.get("VERTEX").offset;
            for (int i = 0; i < p.length; i += groupSize) {
                g.indexBuffer.add(p[i + vertexOffset]);
            }
            if (!fakeTextureMapping) {
                // texture coordinate index buffer for vertices
                g.texIndexBuffer = new ArrayList<Integer>();
                int textureOffset = inputMap.get("TEXCOORD").offset;
                for (int i = 0; i < p.length; i += groupSize) {
                    g.texIndexBuffer.add(p[i + textureOffset]);
                }
            }
            // normal index buffer for vertices
            g.normalIndexBuffer = new ArrayList<Integer>();
            int normalOffset = inputMap.get("NORMAL").offset;
            for (int i = 0; i < p.length; i += groupSize) {
                g.normalIndexBuffer.add(p[i + normalOffset]);
            }

            // TODO animation parsing goes here
            
            // post processing collada data:
            createD3DVertexList(g, null /*skinnedAnis*/);
            
            collada.geometryMap.put(g.meshName, g);
    	}
	}

	private void loadControllers(Element el) {
    	List<Element> le = getChildElements(el);
    	// load all controllers:
    	for (Element e : le) {
    		assert("controller".equals(e.getTagName()));
    		Controller c = new Controller();
    		c.name = e.getAttribute("id");
    		collada.controllerMap.put(c.name, c);
    		Element skin = getSingleAssertedChildElement(e, "skin");
    		// TODO load skin like in parseSkinnedAnimations()
//    		String meshId = getInternalAttribute(skin, "source");
//    		Geometry g = collada.geometryMap.get(meshId);
//    		assert(g != null);
    		//log(" with geometry: " + meshId);
    	}
	}

	private void loadVisualScene(String ivsURL, Element docEl) {
    	Element el = getSingleAssertedChildElement(docEl, "library_visual_scenes");
    	el = getChildElement(el, "visual_scene", "id", ivsURL, true);
    	collada.sceneName = ivsURL;
    	log(" Parsing Scene " + collada.sceneName);
    	List<Element> le = getChildElements(el);
    	// load all nodes of the scene:
    	for (Element e : le) {
    		//log(e.getNodeName());
    		//loadNode(e);
    		parseNodeHierarchy(e, collada.colladaNodeList, -1);
    	}
	}

    // depending on node type load specific values or continue tree parsing
	private boolean loadNode(Element e) {
		//log(" parse node: " + e.getAttribute("id"));
		if (e.getAttribute("type").equalsIgnoreCase("JOINT")) {
			loadBoneHierarchy(e);
			return true;
		}
		Element ic = getSingleAssertedChildElement(e, "instance_controller");
		if (ic != null) {
			String controllerId = getInternalURL(ic);
			log(" found instance_controller: " + controllerId);
			RenderObject ro = new RenderObject(); 
			Element skEl = getSingleAssertedChildElement(ic, "skeleton");
			if (skEl != null) {
				String skelId = getInternalText(skEl);
				List<Bone> root = collada.boneRootsMap.get(skelId);
				assert(root != null);
				ro.boneHierarchyId = skelId;
				log(" using skeleton: " + skelId);
			}
			assert(collada.controllerMap.get(controllerId) != null);
		}
		return false;
	}

	// parse node hierarchy, -1 as parent id means top level
	// referencing a non-existing id is an error
    private void parseNodeHierarchy(Element node_el, List<ColladaNode> nodes, int parentId) {
    	boolean done = loadNode(node_el);
    	if (done) return;
        // recursively parse node hierarchy
        ColladaNode node = new ColladaNode();
        node.id = nodes.size();
        node.parentId = parentId;
        assert(parentId < node.id);
        node.name = node_el.getAttribute("id");
        //log(" parsing node: " + node.name);
        nodes.add(node);
//      NodeList children = bone_el.getElementsByTagName("node");
//      for (int i = 0; i < children.getLength(); i++) {
//          parseBoneHierarchy((Element)children.item(i), bones, bone.id);
//      }
      List<Element> children = getChildElements(node_el);
      for (Element e : children) {
          if (e.getNodeName().equals("node"))
              parseNodeHierarchy(e, nodes, node.id);
      }
    }

	private void loadBoneHierarchy(Element e) {
		String rootBoneId = e.getAttribute("id"); 
		log(" parse bone hierarchy from root: " + rootBoneId);
		
		List<Bone> bones = new ArrayList<>();
		parseBoneHierarchy(e, bones , -1);
		collada.boneRootsMap.put(rootBoneId, bones);
		log("  --> loaded bones: " + bones.size());
		//printBoneHierarchy(bones);
	}

	private List<Element> getChildElements(Element el) {
		List<Element> ret = new ArrayList<>(); 
    	NodeList nl = el.getChildNodes();
    	for (int i = 0; i < nl.getLength(); i++) {
    		Node n = nl.item(i);
    		if (n instanceof Element) {
    			ret.add((Element)n);
    		}
    	}
    	return ret;
	}

	private void log(String string) {
		System.out.println(string);
	}

	// getInternal means verify and discard leading # in id references
	private String getInternalAttribute(Element el, String attributeName) {
    	String attr = el.getAttribute(attributeName);
    	assert (attr != null && attr.startsWith("#"));
		return attr.substring(1);
	}

	// getInternal means verify and discard leading # in id references
	private String getInternalURL(Element el) {
		return getInternalAttribute(el, "url");
	}

	// getInternal means verify and discard leading # in id references
	private String getInternalText(Element el) {
    	String id = el.getTextContent();
    	assert (id != null && id.startsWith("#"));
		return id.substring(1);
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

    private List<SkinnedAnimation> parseSkinnedAnimations(String mesh_name, Document doc, Geometry mesh) {
        List<SkinnedAnimation> anis = new ArrayList<SkinnedAnimation>();
        System.out.println("Skinned Animations for " + mesh_name + ":");
        NodeList library_visual_scenes = doc.getElementsByTagName("library_visual_scenes");
        if (library_visual_scenes.getLength() > 1) {
            System.out.println("WARNING found multiple scene libraries: " + library_visual_scenes.getLength() +  " Only first will be used");
        }
        Element vis_scene = (Element)library_visual_scenes.item(0);
        Element meshNode = (Element) getChildElement(vis_scene, "node", "id", mesh_name, false);
        NodeList inst_controllers = vis_scene.getElementsByTagName("instance_controller");
        if (inst_controllers.getLength() > 1) {
            System.out.println("WARNING found multiple instance_controllers: " + inst_controllers.getLength() +  " Only first will be used");
        }
        Element inst_controller = (Element) inst_controllers.item(0);
        if (inst_controller == null) {
            // no skinned animations available
            return anis;
        }
        String instControllerName = inst_controller.getAttribute("url").substring(1);
        Element skeleton = (Element) inst_controller.getElementsByTagName("skeleton").item(0);
        assert(skeleton.getTextContent().startsWith("#")); // we only can handle internal links 
        String skeletonName = skeleton.getTextContent().substring(1);  
        System.out.println("found controller: " + instControllerName +  " skeleton: " + skeletonName);
        // find root bone by skeletonName inside this visual_scene
        Element bonesNode = getChildElement(vis_scene, "node", "id", skeletonName, false);
        if (bonesNode != null) {
            String node_name = bonesNode.getAttributes().getNamedItem("id").getTextContent();
            System.out.println("Bone info here: " + node_name);
        }
        //Element scene = (Element) vis_scene.getElementsByTagName("visual_scene").item(0);
//        for (int temp = 0; temp < vis_scene.getChildNodes().getLength(); temp++) {
//            Node child = vis_scene.getChildNodes().item(temp);
//            if (child.getNodeType() != Node.ELEMENT_NODE) continue;
//            Element child_el = (Element) child;
//            System.out.println(" vis_scene child: " + child_el.getNodeName());
//            String node_name = child.getAttributes().getNamedItem("id").getTextContent();
//            //if (instControllerName.startsWith(node_name)) {
//            if (skeletonName.equals(node_name)) {
//                System.out.println("Bone info here: " + node_name);
//                bonesNode = child_el;
//            }
//        }
        // bones_node should be directly the root bone, following search is no longer necessary
        /*NodeList possibleRootBones = bonesNode.getChildNodes();//bonesNode.getElementsByTagName("node");
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
        } */       
        parseBoneHierarchy(bonesNode, boneList, -1);
        assertBoneHierarchy(boneList);
        printBoneHierarchy(boneList);
        
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
        // joint names not used at all
//        String[] jointNames = parse_strings(flatJointNames);
//        for (int i = 0; i < jointNames.length; i++) {
//            Joint j = new Joint();
//            a.joints.add(j);
//            j.name = jointNames[i];
//            assert a.joints.get(i).name.equals(boneList.get(i).name); // if this differs we need mapping
//        }
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
        assert bones.get(0).parentId == -1;  // root is at index 0
        for (int i = 1; i < bones.size(); i++) {
            assert bones.get(i).parentId < i;
        }
    }

    // print table of bone / parent dependencies
    private void printBoneHierarchy(List<Bone> bones) {
    	System.out.println("| Bone #  | Parent #|");
        for (int i = 0; i < bones.size(); i++) {
        	System.out.println(String.format("|%8d |%8d |", bones.get(i).id, bones.get(i).parentId));
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
        private ColladaImport2 getOuterType() {
            return ColladaImport2.this;
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

    // Bone and Node class are essentially tree structures.
    // The depend on adding only nodes that have a parent that was already added before
    // This leads to simple List storage and parent references via index. 
    private class ColladaNode {
        int id;
        int parentId;
        String name;
    }

    private class Bone extends ColladaNode {
        public List<Animation> transformations;
        Float[] bindPose;
    }

    // store bone tree
    private List<Bone> boneList = new ArrayList<>();

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

    private void createD3DVertexList(Geometry mesh, List<SkinnedAnimation> animations) {
        Random rand = new Random();
        int numVertices = mesh.indexBuffer.size();
        SkinnedAnimation ani = findAnimation(mesh.meshName, animations);
        boolean isAnimated = ani != null;
        assert mesh.indexBuffer.size() / 3 == mesh.numFaces;
        if (!fakeTextureMapping)
            assert mesh.indexBuffer.size() == mesh.texIndexBuffer.size();
        assert mesh.verts.length / 3 <= numVertices;
        assert mesh.normals.length / 3 <= numVertices;
        if (!fakeTextureMapping) {
            //assert mesh.texcoords.length / 2 == numVertices;
        	if (mesh.texcoords.length / 2 != numVertices) {
        		// maybe we reuse texcoords: make sure by searching max index
        		Integer mx = Collections.max(mesh.texIndexBuffer);
        		mx *= 2;
        		if (mx >= mesh.texcoords.length) {
        			fail("invalid texture coordinates");
        		}
        	}
        }
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
    	if (animations == null) return null;
        for (SkinnedAnimation skinnedAnimation : animations) {
            if (skinnedAnimation.meshName.equals(meshName))
                    return skinnedAnimation;
        }
        return null;
    }

    // parse single input element and put attributes to map:
    // map SEMANTIC to input_attributes (semantic, offset, source)
    private void parseInput(Element input, Map<String, input_el> inputMap) {
        String semantic = input.getAttribute("semantic");
        String source = input.getAttribute("source");
        String offset = input.getAttribute("offset");
        try {
            log("parse semantic: " + semantic);
        } catch (IllegalArgumentException e) {
            // we don't parse unknown input types - just return
            return;
        }
        if (source != null && source.length() > 0) {
            if (source.startsWith("#"))
                source = source.substring(1);
            //inputTypes[cur.ordinal()].source = source;
            log(" parse source: " + source);
        }
        int intOffset = 0;
        if (offset != null && offset.length() > 0) {
            intOffset = Integer.valueOf(offset);
            //inputTypes[cur.ordinal()].offset = intOffset;
            log(" parse offset: " + intOffset);
        }
        input_el inputEl = new input_el();
        inputEl.type = semantic;
        inputEl.offset = intOffset;
        inputEl.source = source;
        inputMap.put(semantic, inputEl);
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

    private Element getSingleAssertedChildElement(Element parent, String tagname) {
    	return getDirectSingleAssertedChildElement(parent, tagname);
//        NodeList list = parent.getElementsByTagName(tagname);
//        assert(list != null && list.getLength() == 1);
//        return (Element) list.item(0);
    }

    private Element getDirectSingleAssertedChildElement(Element parent, String tagname) {
    	List<Element> list = getChildElements(parent);
//    	log("---");
//    	for ( Element e : list) {
//    		log(" child: " + e.getTagName());
//    	}
//    	log(":---");
    	for ( Element e : list) {
    		if (e.getTagName().equals(tagname)) {
    			return e;
    		}
    	}
        return null;
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

//    input_el[] getEmptyInputTable() {
//        int len = input_el_type.values().length;
//        input_el[] in = new input_el[len];
//        for (int i = 0; i < len; i++) {
//            in[i] = new input_el();
//            in[i].type = input_el_type.values()[i];
//            in[i].offset = 0;
//            in[i].source = null;
//        }
//        return in;
//    }
    // local data structures
    enum input_el_type { POSITION, VERTEX, NORMAL, TEXCOORD};
    private class input_el {
        String type;
        int offset;
        String source;
    }
    
    // write Output file
    void writeOutputFile(File outfile) {
        // write binary file:
        //ObjectOutputStream oos = new ObjectOutputStream(new BufferedOutputStream(new FileOutputStream(outfile)));
        DataOutputStream oos;
		try {
			oos = new DataOutputStream(new BufferedOutputStream(new FileOutputStream(outfile)));
	        Set<Entry<String, Geometry>> geom = collada.geometryMap.entrySet();
	        // first entry in bin file is number of entries:
	        writeInt(oos, geom.size());
	        for (Entry<String, Geometry> g : geom) {
	        	Geometry mesh = g.getValue();
	        	log("writing geometry: " + mesh.meshName);
	        	// add mesh name to binary file
                writeInt(oos, mesh.meshName.length());
                writeString(oos, mesh.meshName);
	            List<D3DVertex> all = mesh.d3dVertexList;
	            boolean isSkinned = false;
	            // write type: 0 means not skinned, 1 means skinned
	            if (all.size() > 0 && all.get(0).weight != null) {
	                writeInt(oos, 1);
	                isSkinned = true;
	            } else {
	                writeInt(oos, 0);
	            }
	            /*        if (isSkinned) {
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
	        }*/
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
	        if (true/*anis.size() == 0*/) {
	            // write 0 to indicate that no animation is present
	            writeInt(oos, 0);
	        } else {
	            /*String ani_name = node_name;
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
	            } */
	        }
	        
	        // write number of available animations
	        //oos.writeFloat(pi); // uses big endian
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
	    oos.close();
	    	
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    }
}


