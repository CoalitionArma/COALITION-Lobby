#ifdef WORKBENCH
//------------------------------------------------------------------------------------------------
//! World Editor preview of a COA_ManualMarker: the icon is draped on the terrain in its colour,
//! size and rotation, as the in-game map draws it. Selecting the marker adds an arrow and a
//! readout of the bearing it points on the map.
//!
//! The map rotates the icon clockwise by the entity's yaw. Direction icons (arrows) are drawn
//! facing east, so they point at yaw + 90 - not along the entity's forward axis.
class COA_ManualMarkerPreview
{
	//! Draped mesh resolution: GRID x GRID cells. The vertex/index array sizes below follow from it.
	protected static const int GRID = 16;
	protected static const int VERTEX_COUNT = 289;		// (GRID + 1)^2
	protected static const int INDEX_COUNT = 1536;		// GRID^2 * 6
	protected static const float LIFT = 0.3;

	//! Pixel-sized markers keep their screen size in game, so the preview scales with camera distance.
	//! Sizes are in 1080p reference pixels; the editor camera is assumed to have a 60 degree vertical FOV.
	protected static const float REFERENCE_SCREEN_HEIGHT = 1080;
	protected static const float HALF_FOV_TAN = 0.577;

	protected static const string MATERIAL_DIRECTORY = "$profile:COA_MarkerPreview";

	//! Material files written this session
	protected static ref set<string> s_aWrittenMaterials;

	protected COA_ManualMarker m_Marker;
	protected ref Resource m_MeshResource;

	protected bool m_bHasImage;
	protected string m_sMaterial;
	protected float m_fU0, m_fV0, m_fU1 = 1, m_fV1 = 1;
	protected float m_fAspect = 1;						// height / width

	protected vector m_vBuiltOrigin;
	protected float m_fBuiltYaw;
	protected float m_fBuiltWidth;

	//------------------------------------------------------------------------------------------------
	void COA_ManualMarkerPreview(COA_ManualMarker marker)
	{
		m_Marker = marker;
		if (!m_Marker.m_bJustText && LoadImage())
			m_bHasImage = CreateMaterial();
	}

	//------------------------------------------------------------------------------------------------
	void Update(bool selected)
	{
		if (m_Marker.m_bJustText)
		{
			DrawText(m_Marker.GetDescription(), m_Marker.GetOrigin());
			return;
		}

		float width = GetWidth();
		if (m_bHasImage)
		{
			vector origin = m_Marker.GetOrigin();
			float yaw = m_Marker.GetYawPitchRoll()[0];
			bool moved = vector.DistanceSq(origin, m_vBuiltOrigin) > 0.0001 || Math.AbsFloat(yaw - m_fBuiltYaw) > 0.01;
			if (!m_MeshResource || moved || Math.AbsFloat(width - m_fBuiltWidth) > m_fBuiltWidth * 0.05)
				BuildMesh(width);
			else
				ApplyMesh();
		}

		if (selected)
			DrawSelection(width);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 IMAGE
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Finds the texture and the quad's UV rectangle - a plain .edds uses the whole texture
	protected bool LoadImage()
	{
		ResourceName imageSet = m_Marker.GetImageSet();
		if (imageSet.IsEmpty())
			return false;

		if (imageSet.EndsWith(".edds"))
		{
			m_sMaterial = imageSet;
			return true;
		}

		array<string> lines = SCR_FileIOHelper.ReadFileContent(imageSet.GetPath(), false);
		if (!lines)
		{
			Print("COA_ManualMarkerPreview: can't read " + imageSet, LogLevel.WARNING);
			return false;
		}

		string quadName = m_Marker.GetQuadName();
		string texturePath;
		float refX, refY, posX, posY, sizeX, sizeY;
		bool inQuad, found;
		array<string> parts = {};
		foreach (string line : lines)
		{
			line.TrimInPlace();
			parts.Clear();
			line.Split(" ", parts, true);
			if (parts.IsEmpty())
				continue;

			string head = parts[0];
			if (head == "RefSize" && parts.Count() >= 3)
			{
				refX = parts[1].ToFloat();
				refY = parts[2].ToFloat();
			}
			else if (head == "path" && texturePath.IsEmpty())
			{
				texturePath = GetQuoted(line);
			}
			else if (head == "ImageSetDefClass" && parts.Count() >= 2)
			{
				string name = GetQuoted(line);
				if (name.IsEmpty())
					name = parts[1];

				inQuad = name == quadName;
			}
			else if (inQuad && head == "Pos" && parts.Count() >= 3)
			{
				posX = parts[1].ToFloat();
				posY = parts[2].ToFloat();
			}
			else if (inQuad && head == "Size" && parts.Count() >= 3)
			{
				sizeX = parts[1].ToFloat();
				sizeY = parts[2].ToFloat();
				found = true;
			}
			else if (head == "}")
			{
				inQuad = false;
			}
		}

		if (!found || texturePath.IsEmpty() || refX <= 0 || refY <= 0 || sizeX <= 0)
		{
			Print(string.Format("COA_ManualMarkerPreview: no quad \"%1\" in %2", quadName, imageSet), LogLevel.WARNING);
			return false;
		}

		// Older imagesets store a bare path
		if (!texturePath.StartsWith("{"))
			texturePath = Workbench.GetResourceName(texturePath);

		m_sMaterial = texturePath;
		m_fU0 = posX / refX;
		m_fV0 = posY / refY;
		m_fU1 = (posX + sizeX) / refX;
		m_fV1 = (posY + sizeY) / refY;
		m_fAspect = sizeY / sizeX;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static string GetQuoted(string line)
	{
		int start = line.IndexOf("\"");
		int end = line.LastIndexOf("\"");
		if (start == -1 || end <= start)
			return string.Empty;

		return line.Substring(start + 1, end - start - 1);
	}

	//------------------------------------------------------------------------------------------------
	//! MeshObject.Create only loads materials from .emat files (a Material.Create name isn't found),
	//! so each texture/colour pair gets a generated file in the profile folder.
	//! UI atlases are white with the shape in alpha, so the marker colour tints both the base colour
	//! and the emissive (map icons are unlit). On entry m_sMaterial holds the texture.
	protected bool CreateMaterial()
	{
		if (!s_aWrittenMaterials)
			s_aWrittenMaterials = new set<string>();

		ResourceName texture = m_sMaterial;
		Color color = m_Marker.GetColor();
		string path = string.Format("%1/%2_%3.emat", MATERIAL_DIRECTORY, texture.Hash(), color.PackToInt());
		m_sMaterial = path;
		if (s_aWrittenMaterials.Contains(path))
			return true;

		FileIO.MakeDirectory(MATERIAL_DIRECTORY);
		FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
		if (!file)
		{
			Print("COA_ManualMarkerPreview: can't write " + path, LogLevel.WARNING);
			return false;
		}

		string colorText = string.Format("%1 %2 %3 1", color.R(), color.G(), color.B());
		file.WriteLine("MatPBRBasic {");
		file.WriteLine(" Sort translucent");
		file.WriteLine(" Cull none");
		file.WriteLine(" BlendMode AlphaBlend");
		file.WriteLine(" AlphaTest 0.05");
		file.WriteLine(" Color " + colorText);
		file.WriteLine(" Emissive " + colorText);
		file.WriteLine(" EmissiveLV 1");
		file.WriteLine(" EmissiveAbsoluteLV 0");
		file.WriteLine(" BCRMap \"" + texture + "\"");
		file.WriteLine(" OpacityMap \"" + texture + "\"");
		file.WriteLine(" EmissiveMap \"" + texture + "\"");
		file.WriteLine("}");
		file.Close();

		s_aWrittenMaterials.Insert(path);
		return true;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MESH
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected float GetWidth()
	{
		if (m_Marker.GetUseWorldScale())
			return m_Marker.GetSize();

		vector camera[4];
		m_Marker.GetWorld().GetCurrentCamera(camera);
		float distance = vector.Distance(camera[3], m_Marker.GetOrigin());
		return m_Marker.GetSize() * distance * 2 * HALF_FOV_TAN / REFERENCE_SCREEN_HEIGHT;
	}

	//------------------------------------------------------------------------------------------------
	//! Map space is yaw only: pitch and roll don't rotate the icon in game
	protected static void GetMapAxes(float yaw, out vector right, out vector up)
	{
		float rad = yaw * Math.DEG2RAD;
		right = Vector(Math.Cos(rad), 0, -Math.Sin(rad));
		up = Vector(Math.Sin(rad), 0, Math.Cos(rad));
	}

	//------------------------------------------------------------------------------------------------
	protected vector GetGroundPoint(vector pos)
	{
		BaseWorld world = m_Marker.GetWorld();
		pos[1] = Math.Max(world.GetSurfaceY(pos[0], pos[2]), world.GetOceanBaseHeight()) + LIFT;
		return pos;
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildMesh(float width)
	{
		vector origin = m_Marker.GetOrigin();
		float yaw = m_Marker.GetYawPitchRoll()[0];
		float height = width * m_fAspect;
		vector right, up;
		GetMapAxes(yaw, right, up);

		vector verts[289];
		float uvs[578];
		int indices[1536];

		int vI, uI;
		for (int row = 0; row <= GRID; row++)
		{
			float v = row / (float)GRID;
			for (int column = 0; column <= GRID; column++)
			{
				float u = column / (float)GRID;
				vector pos = origin + right * ((u - 0.5) * width) + up * ((0.5 - v) * height);
				verts[vI] = m_Marker.CoordToLocal(GetGroundPoint(pos));
				vI++;

				uvs[uI] = Math.Lerp(m_fU0, m_fU1, u);
				uI++;
				uvs[uI] = Math.Lerp(m_fV0, m_fV1, v);
				uI++;
			}
		}

		int iI;
		for (int row = 0; row < GRID; row++)
		{
			for (int column = 0; column < GRID; column++)
			{
				int topLeft = row * (GRID + 1) + column;
				int bottomLeft = topLeft + GRID + 1;
				indices[iI] = topLeft;			iI++;
				indices[iI] = bottomLeft;		iI++;
				indices[iI] = topLeft + 1;		iI++;
				indices[iI] = topLeft + 1;		iI++;
				indices[iI] = bottomLeft;		iI++;
				indices[iI] = bottomLeft + 1;	iI++;
			}
		}

		if (!m_MeshResource)
		{
			int numVertices[] = {VERTEX_COUNT};
			int numIndices[] = {INDEX_COUNT};
			string materials[1] = {m_sMaterial};
			m_MeshResource = MeshObject.Create(1, numVertices, numIndices, materials, 0);
		}

		MeshObject mesh = m_MeshResource.GetResource().ToMeshObject();
		mesh.UpdateVerts(0, verts, uvs);
		mesh.UpdateIndices(0, indices);
		ApplyMesh();

		m_vBuiltOrigin = origin;
		m_fBuiltYaw = yaw;
		m_fBuiltWidth = width;
	}

	//------------------------------------------------------------------------------------------------
	//! Replaces the prefab's axis gizmo, which shows the entity's forward axis rather than where the icon points
	protected void ApplyMesh()
	{
		MeshObject mesh = m_MeshResource.GetResource().ToMeshObject();
		if (m_Marker.GetVObject() != mesh)
			m_Marker.SetObject(mesh, "");
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 SELECTION OVERLAY
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void DrawSelection(float width)
	{
		vector origin = m_Marker.GetOrigin();
		float yaw = m_Marker.GetYawPitchRoll()[0];
		float height = width * m_fAspect;
		vector right, up;
		GetMapAxes(yaw, right, up);

		int color = m_Marker.GetColor().PackToInt() | 0xFF000000;
		ShapeFlags flags = ShapeFlags.ONCE | ShapeFlags.NOZBUFFER;

		// Footprint
		vector corners[5];
		corners[0] = GetGroundPoint(origin - right * (width * 0.5) + up * (height * 0.5));
		corners[1] = GetGroundPoint(origin + right * (width * 0.5) + up * (height * 0.5));
		corners[2] = GetGroundPoint(origin + right * (width * 0.5) - up * (height * 0.5));
		corners[3] = GetGroundPoint(origin - right * (width * 0.5) - up * (height * 0.5));
		corners[4] = corners[0];
		Shape.CreateLines(color, flags, corners, 5);

		// Pointing direction: east-facing art rotated clockwise by yaw
		float bearing = Math.Repeat(yaw + 90, 360);
		vector center = GetGroundPoint(origin);
		vector tip = center + right * (width * 0.75);
		float head = width * 0.15;
		vector arrow[6];
		arrow[0] = center;
		arrow[1] = tip;
		arrow[2] = tip;
		arrow[3] = tip - right * head + up * (head * 0.5);
		arrow[4] = tip;
		arrow[5] = tip - right * head - up * (head * 0.5);
		Shape.CreateSeparateLines(color, flags, arrow, 6, 2);

		string title = m_Marker.GetDescription();
		if (title.IsEmpty())
			title = m_Marker.GetQuadName();

		string size;
		if (m_Marker.GetUseWorldScale())
			size = string.Format("%1 m wide", m_Marker.GetSize());
		else
			size = string.Format("%1 px wide - fixed screen size", m_Marker.GetSize());

		int degrees = Math.Round(bearing);
		string text = string.Format("%1\nPoints %2 deg %3\n%4", title, degrees % 360, GetCompassPoint(bearing), size);
		DrawText(text, tip);
	}

	//------------------------------------------------------------------------------------------------
	protected void DrawText(string text, vector position)
	{
		if (text.IsEmpty())
			return;

		position = GetGroundPoint(position);
		DebugTextWorldSpace.Create(m_Marker.GetWorld(), text, DebugTextFlags.ONCE | DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA,
			position[0], position[1] + 2, position[2], 14, 0xFFFFFFFF, 0xC0000000);
	}

	//------------------------------------------------------------------------------------------------
	protected static string GetCompassPoint(float bearing)
	{
		array<string> points = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
		int index = Math.Round(bearing / 45);
		return points[index % 8];
	}
}
#endif
