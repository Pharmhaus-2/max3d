/*
Max3D
Copyright (c) 2008, Mark Sibly
All rights reserved.

Redistribution and use in source and binary forms, with or without
conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of Max3D's copyright owner nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include "std.h"

#include "app.h"
#include "modelutil.h"

#include <assimp.hpp>
#include <aiScene.h>
#include <aiPostProcess.h>

static CVec3 scale( 1.0f );

static vector<CMaterial*> materials;		//same index as scene
static vector<CModelSurface*> surfaces;	//same index as scene

static CVec3 Vec3( const aiVector3D &v ){
	return CVec3( v.x,v.y,v.z );
}

static CQuat Quat( const aiQuaternion &q ){
	return CQuat( CVec3( q.x,q.y,q.z ),q.w );
}

static CMaterial *loadMaterial( aiMaterial *mat ){
	CMaterial *cmat=new CMaterial;
	
	aiColor4D color;
	aiString name,path;
	
	if( mat->Get( AI_MATKEY_NAME,name )==AI_SUCCESS ){
		cmat->SetName( name.data );
	}
	
	//Diffuse color/texture
	if( mat->Get( AI_MATKEY_TEXTURE_DIFFUSE(0),path )==AI_SUCCESS ){
		if( !name.length ) cmat->SetName( path.data );
		cmat->SetTexture( "DiffuseTexture",(CTexture*)App.ImportObject( "CTexture",path.data ) );
	}else if( mat->Get( AI_MATKEY_COLOR_DIFFUSE,color )==AI_SUCCESS ){
		cmat->SetColor( "DiffuseColor",CVec3( color.r,color.g,color.b ) );
	}

	if( mat->Get( AI_MATKEY_TEXTURE_SPECULAR(0),path )==AI_SUCCESS ){
		cmat->SetTexture( "SpecularTexture",(CTexture*)App.ImportObject( "CTexture",path.data ) );
	}else if( mat->Get( AI_MATKEY_COLOR_SPECULAR,color )==AI_SUCCESS ){
		cmat->SetColor( "SpecularColor",CVec3( color.r,color.g,color.b ) );
	}
	
	if( mat->Get( AI_MATKEY_TEXTURE_EMISSIVE(0),path )==AI_SUCCESS ){
		cmat->SetTexture( "EmissiveTexture",(CTexture*)App.ImportObject( "CTexture",path.data ) );
	}else if( mat->Get( AI_MATKEY_COLOR_EMISSIVE,color )==AI_SUCCESS ){
		cmat->SetColor( "EmissiveColor",CVec3( color.r,color.g,color.b ) );
	}
	
	if( mat->Get( AI_MATKEY_TEXTURE_NORMALS(0),path )==AI_SUCCESS ){
		cmat->SetTexture( "NormalTexture",(CTexture*)App.ImportObject( "CTexture",path.data ) );
	}

	return cmat;
}

static CModelSurface *loadSurface( const aiMesh *mesh ){
	CModelSurface *surf=new CModelSurface;
	surf->SetMaterial( materials[mesh->mMaterialIndex] );	
	
	aiVector3D *mv=mesh->mVertices;
	aiVector3D *mn=mesh->mNormals;
	aiVector3D *mc=mesh->mTextureCoords[0];
	
	for( int i=0;i<mesh->mNumVertices;++i ){
		CVertex v;
		v.position=*(CVec3*)mv++ * scale;
		if( mn ) memcpy( &v.normal,mn++,12 );
		if( mc ) memcpy( &v.texcoords[0],mc++,8 );
		surf->AddVertex( v );
	}

	aiFace *face=mesh->mFaces;
	for( int i=0;i<mesh->mNumFaces;++i ){
		surf->AddTriangle( face->mIndices[0],face->mIndices[1],face->mIndices[2] );
		++face;
	}
	
	if( !mn ) surf->UpdateNormals();
	if( mc ) surf->UpdateTangents();

	return surf;
}

static CModel *loadModel( const aiNode *node,CModel *parent ){
	
	CModel *model=new CModel;
	model->SetParent( parent );
	
	aiVector3D scaling;
	aiQuaternion rotation;
	aiVector3D position;
	node->mTransformation.Decompose( scaling,rotation,position );
	
//	cout<<"Position="<<Vec3( position )<<"Scaling="<<Vec3(scaling)<<endl;
	
	model->SetTRS( Vec3( position ) * scale,Quat( rotation ),Vec3( scaling ) );
	
	if( node->mNumMeshes ){
		for( int i=0;i<node->mNumMeshes;++i ){
			CModelSurface *surf=surfaces[ node->mMeshes[i] ];
			model->AddSurface( surf );
		}
	}
	
	for( int i=0;i<node->mNumChildren;++i ){
		loadModel( node->mChildren[i],model );
	}

	return model;
}

CModel *CModelUtil::ImportModel( const string &path,int collType,float mass ){

	int flags=
	aiProcess_Triangulate |
	aiProcess_JoinIdenticalVertices |
	aiProcess_FindDegenerates |
	aiProcess_ImproveCacheLocality |
	aiProcess_SortByPType |
	aiProcess_ConvertToLeftHanded |
	0;

	Assimp::Importer importer;
	importer.SetPropertyInteger( AI_CONFIG_PP_SBP_REMOVE,aiPrimitiveType_LINE|aiPrimitiveType_POINT ); 	

	const aiScene *scene=importer.ReadFile( path,flags );

	string err=importer.GetErrorString();
	if( err.size() ) Warning( "aiImporter error:"+err );
	if( !scene ) return 0;

	cout<<"ImportModel, path="<<path<<", numAnimations="<<scene->mNumAnimations<<endl;
	for( int i=0;i<scene->mNumAnimations;++i ){
		aiAnimation *anim=scene->mAnimations[i];
		cout<<"Animation "<<i<<", name="<<anim->mName.data<<", duration="<<anim->mDuration<<", NumChannels="<<anim->mNumChannels<<endl;
	}

	materials.clear();
	for( int i=0;i<scene->mNumMaterials;++i ){
		materials.push_back( loadMaterial( scene->mMaterials[i] ) );
	}
	
	for( int i=0;i<scene->mNumMeshes;++i ){
		surfaces.push_back( loadSurface( scene->mMeshes[i] ) );
	}
	
	CModel *model=loadModel( scene->mRootNode,0 );
	
	for( int i=0;i<materials.size();++i ){
		materials[i]->Release();
	}
	materials.clear();
	
	for( int i=0;i<surfaces.size();++i ){
		surfaces[i]->Release();
	}
	surfaces.clear();
	
	if( collType || mass ) model->SetBody( CreateModelBody( model,collType,mass ) );
	
	return model;
}

/*
CModel *CModelUtil::ImportModel( const string &path,int collType,float mass ){
	
	int flags=
	aiProcess_Triangulate |
	aiProcess_JoinIdenticalVertices |
	aiProcess_PreTransformVertices |
	aiProcess_FindDegenerates |
	aiProcess_ImproveCacheLocality |
	aiProcess_SortByPType |
	aiProcess_ConvertToLeftHanded |
	0;

	Assimp::Importer importer;
	importer.SetPropertyInteger( AI_CONFIG_PP_SBP_REMOVE,aiPrimitiveType_LINE|aiPrimitiveType_POINT ); 	

	const aiScene *scene=importer.ReadFile( path,flags );
	
	cout<<"ImportModel, path="<<path<<", numAnimations="<<scene->mNumAnimations<<endl;
	for( int i=0;i<scene->mNumAnimations;++i ){
		aiAnimation *anim=scene->mAnimations[i];
		cout<<"Animation "<<i<<", name="<<anim->mName.data<<", duration="<<anim->mDuration<<", NumChannels="<<anim->mNumChannels<<endl;
	}

	string err=importer.GetErrorString();
	if( err.size() ) Warning( "aiImporter error:"+err );
	if( !scene ) return 0;

	vector<CMaterial*> mats;
	
	for( int i=0;i<scene->mNumMaterials;++i ){
		aiMaterial *mat=scene->mMaterials[i];
		CMaterial *cmat=loadMaterial( mat );
		mats.push_back( cmat );
	}
	
	CModel *model=new CModel;
	
	for( int i=0;i<scene->mNumMeshes;++i ){
		aiMesh *mesh=scene->mMeshes[i];

		CModelSurface *surf=loadSurface( mesh );
	
		surf->SetMaterial( mats[mesh->mMaterialIndex] );
	
		model->AddSurface( surf );

		surf->Release();
	}

	for( int i=0;i<mats.size();++i ){
		mats[i]->Release();
	}
	
	if( collType || mass ) model->SetBody( CreateModelBody( model,collType,mass ) );

	return model;
}
*/

static void createBodySurf( CModelSurface *surf,CModel *model ){
	CMat4 mat=model->WorldMatrix();
	CMat4 itMat=(-mat).Transpose();
	for( vector<CModelSurface*>::const_iterator it=model->Surfaces().begin();it!=model->Surfaces().end();++it ){
		CModelSurface *tsurf=(CModelSurface*)( (*it)->Copy() );
		tsurf->Transform( mat,itMat );
		surf->AddSurface( tsurf );
	}
	for( CEntity *child=model->Children();child;child=child->Next() ){
		if( CModel *model=dynamic_cast<CModel*>( child ) ) createBodySurf( surf,model );
	}
}

CBody *CModelUtil::CreateModelBody( CModel *model,int collType,float mass ){
	CModelSurface *physSurf=new CModelSurface;
	createBodySurf( physSurf,model );
	CBody *body=App.World()->Physics()->CreateSurfaceBody( physSurf,collType,mass );
	physSurf->Release();
	return body;
}

CModel *CModelUtil::CreateSphere( CMaterial *material,float radius,int collType,float mass ){
	int segs=8;
	CModelSurface *surf=new CModelSurface;
	surf->SetMaterial( material );
	int segs2=segs*2;
	for( int j=0;j<segs2;++j ){
		CVertex v( 0,radius,0 );
		v.normal=CVec3( 0,1,0 );
		v.texcoords[0]=CVec2( (j+1)/float(segs),0 );
		surf->AddVertex( v );
	}
	for( int i=1;i<segs;++i ){
		float r=sinf( i*PI/segs )*radius;
		float y=cosf( i*PI/segs );
		CVertex v( 0,y*radius,-r );
		v.normal=CVec3( 0,y,-1 ).Normalize();
		v.texcoords[0]=CVec2( 0,i/float(segs) );
		surf->AddVertex( v );
		for( int j=1;j<segs2;++j ){
			float x=sinf( j*TWOPI/segs2 );
			float z=-cosf( j*TWOPI/segs2 );
			CVertex v( x*r,y*radius,z*r );
			v.normal=CVec3( x,y,z ).Normalize();
			v.texcoords[0]=CVec2( j/float(segs),i/float(segs) );
			surf->AddVertex( v );
		}
		v.texcoords[0].x=2;
		surf->AddVertex( v );
	}
	for( int j=0;j<segs2;++j ){
		CVertex v( 0,-radius,0 );
		v.normal=CVec3( 0,-1,0 );
		v.texcoords[0]=CVec2( (j+1)/float(segs),1 );
		surf->AddVertex( v );
	}
	int v=0;
	for( int j=0;j<segs2;++j ){
		surf->AddTriangle( v,v+segs2+1,v+segs2 );
		++v;
	}
	for( int i=1;i<segs-1;++i ){
		for( int j=0;j<segs2;++j ){
			surf->AddTriangle( v,v+1,v+segs2+2 );
			surf->AddTriangle( v,v+segs2+2,v+segs2+1 );
			++v;
		}
		++v;
	}
	for( int j=0;j<segs2;++j ){
		surf->AddTriangle( v,v+1,v+segs2+1 );
		++v;
	}
	surf->UpdateTangents();
	CModel *model=new CModel;
	model->AddSurface( surf );
	surf->Release();
	if( collType || mass ) model->SetBody( App.World()->Physics()->CreateSphereBody( radius,collType,mass ) );
	return model;
}

CModel *CModelUtil::CreateCapsule( CMaterial *material,float radius,float length,int collType,float mass ){
	int segs=8;
	CModelSurface *surf=new CModelSurface;
	surf->SetMaterial( material );
	segs=(segs+1)&~1;
	int segs2=segs*2;
	float hlength=length/2;
	for( int j=0;j<segs2;++j ){
		float ty=hlength+radius;
		CVertex v( 0,ty,0 );
		v.normal=CVec3( 0,1,0 );
		v.texcoords[0]=CVec2( (j+1)/float(segs),0 );
		surf->AddVertex( v );
	}
	for( int i=1;i<segs;++i ){
		float r=sinf( i*PI/segs )*radius;
		float y=cosf( i*PI/segs );
		float ty=y*radius;
		if( i<segs/2 ) ty+=hlength; else ty-=hlength;
		CVertex v( 0,ty,-r );
		v.normal=CVec3( 0,y,-1 ).Normalize();
		v.texcoords[0]=CVec2( 0,i/float(segs) );
		surf->AddVertex( v );
		for( int j=1;j<segs2;++j ){
			float x=sinf( j*TWOPI/segs2 );
			float z=-cosf( j*TWOPI/segs2 );
			float ty=y*radius;
			if( i<segs/2 ) ty+=hlength; else ty-=hlength;
			CVertex v( x*r,ty,z*r );
			v.normal=CVec3( x,y,z ).Normalize();
			v.texcoords[0]=CVec2( j/float(segs),i/float(segs) );
			surf->AddVertex( v );
		}
		v.texcoords[0].x=2;
		surf->AddVertex( v );
	}
	for( int j=0;j<segs2;++j ){
		float ty=-hlength-radius;
		CVertex v( 0,ty,0 );
		v.normal=CVec3( 0,-1,0 );
		v.texcoords[0]=CVec2( (j+1)/float(segs),1 );
		surf->AddVertex( v );
	}
	int v=0;
	for( int j=0;j<segs2;++j ){
		surf->AddTriangle( v,v+segs2+1,v+segs2 );
		++v;
	}
	for( int i=1;i<segs-1;++i ){
		for( int j=0;j<segs2;++j ){
			surf->AddTriangle( v,v+1,v+segs2+2 );
			surf->AddTriangle( v,v+segs2+2,v+segs2+1 );
			++v;
		}
		++v;
	}
	for( int j=0;j<segs2;++j ){
		surf->AddTriangle( v,v+1,v+segs2+1 );
		++v;
	}

	surf->UpdateTangents();
	CModel *model=new CModel;
	model->AddSurface( surf );
	surf->Release();
	if( collType || mass ) model->SetBody( App.World()->Physics()->CreateCapsuleBody( radius,length,collType,mass ) );
	return model;
}

CModel *CModelUtil::CreateCylinder( CMaterial *material,float radius,float length,int collType,float mass ){
	int segs=8;
	CModelSurface *surf=new CModelSurface;
	surf->SetMaterial( material );
	float hlength=length/2;
	CVertex v;
	v.position=CVec3( 0,hlength,-radius );
	v.normal=CVec3( 0,0,-1 );
	v.texcoords[0]=CVec2( 0,0 );
	surf->AddVertex( v );
	v.position=CVec3( 0,-hlength,-radius );
	v.normal=CVec3( 0,0,-1 );
	v.texcoords[0]=CVec2( 0,1 );
	surf->AddVertex( v );
	for( int i=1;i<segs;++i ){
		float x=sinf( i*TWOPI/segs );
		float z=-cosf( i*TWOPI/segs );
		CVertex v;
		v.position=CVec3( x*radius,hlength,z*radius );
		v.normal=CVec3( x,0,z );
		v.texcoords[0]=CVec2( i/float(segs),0 );
		surf->AddVertex( v );
		v.position=CVec3( x*radius,-hlength,z*radius );
		v.normal=CVec3( x,0,z );
		v.texcoords[0]=CVec2( i/float(segs),1 );
		surf->AddVertex( v );
	}
	v.position=CVec3( 0,hlength,-radius );
	v.normal=CVec3( 0,0,-1 );
	v.texcoords[0]=CVec2( 1,0 );
	surf->AddVertex( v );
	v.position=CVec3( 0,-hlength,-radius );
	v.normal=CVec3( 0,0,-1 );
	v.texcoords[0]=CVec2( 1,1 );
	surf->AddVertex( v );
	for( int i=0;i<segs;++i ){
		surf->AddTriangle( i*2,i*2+2,i*2+1 );
		surf->AddTriangle( i*2+2,i*2+3,i*2+1 );
	}
	
	surf->UpdateTangents();
	CModel *model=new CModel;
	model->AddSurface( surf );
	surf->Release();
	if( collType || mass ) model->SetBody( App.World()->Physics()->CreateCylinderBody( radius,length,collType,mass ) );
	return model;
}

CModel *CModelUtil::CreateBox( CMaterial *material,float width,float height,float depth,int collType,float mass ){
	CModelSurface *surf=new CModelSurface;
	surf->SetMaterial( material );
	static const float faces[]={
		0,0,-1,
		-1,+1,-1,0,0,
		+1,+1,-1,1,0,
		+1,-1,-1,1,1,
		-1,-1,-1,0,1,
		1,0,0,
		+1,+1,-1,0,0,
		+1,+1,+1,1,0,
		+1,-1,+1,1,1,
		+1,-1,-1,0,1,
		0,0,1,
		+1,+1,+1,0,0,
		-1,+1,+1,1,0,
		-1,-1,+1,1,1,
		+1,-1,+1,0,1,
		-1,0,0,
		-1,+1,+1,0,0,
		-1,+1,-1,1,0,
		-1,-1,-1,1,1,
		-1,-1,+1,0,1,
		0,1,0,
		-1,+1,+1,0,0,
		+1,+1,+1,1,0,
		+1,+1,-1,1,1,
		-1,+1,-1,0,1,
		0,-1,0,
		-1,-1,-1,0,0,
		+1,-1,-1,1,0,
		+1,-1,+1,1,1,
		-1,-1,+1,0,1
	};
	float hwidth=width/2,hheight=height/2,hdepth=depth/2;
	int n=0,t=0;
	for( int i=0;i<6;++i ){
		CVec3 normal=CVec3( faces[t+0],faces[t+1],faces[t+2] );
		t+=3;
		for( int j=0;j<4;++j ){
			CVertex v( faces[t+0]*hwidth,faces[t+1]*hheight,faces[t+2]*hdepth );
			v.normal=normal;
			v.texcoords[0]=CVec2( faces[t+3],faces[t+4] );
			surf->AddVertex( v );
			t+=5;
		}
		surf->AddTriangle( n,n+1,n+2 );
		surf->AddTriangle( n,n+2,n+3 );
		n+=4;
	}
	
	surf->UpdateTangents();
	CModel *model=new CModel;
	model->AddSurface( surf );
	surf->Release();
	if( collType || mass ) model->SetBody( App.World()->Physics()->CreateBoxBody( width,height,depth,collType,mass ) );
	return model;
}
