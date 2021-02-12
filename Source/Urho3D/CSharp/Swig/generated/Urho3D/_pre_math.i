%ignore Urho3D::M_PI;
%csconst(1) Urho3D::M_PI;
%constant float MPi = (float)3.14159274;
%constant float MHalfPi = Urho3D::M_HALF_PI;
%ignore Urho3D::M_HALF_PI;
%ignore Urho3D::M_MIN_INT;
%csconst(1) Urho3D::M_MIN_INT;
%constant int MMinInt = 2147483648;
%ignore Urho3D::M_MAX_INT;
%csconst(1) Urho3D::M_MAX_INT;
%constant int MMaxInt = 2147483647;
%ignore Urho3D::M_MIN_UNSIGNED;
%csconst(1) Urho3D::M_MIN_UNSIGNED;
%constant unsigned int MMinUnsigned = 0;
%ignore Urho3D::M_MAX_UNSIGNED;
%csconst(1) Urho3D::M_MAX_UNSIGNED;
%constant unsigned int MMaxUnsigned = 4294967295;
%ignore Urho3D::M_EPSILON;
%csconst(1) Urho3D::M_EPSILON;
%constant float MEpsilon = (float)9.99999997E-7;
%ignore Urho3D::M_LARGE_EPSILON;
%csconst(1) Urho3D::M_LARGE_EPSILON;
%constant float MLargeEpsilon = (float)4.99999987E-5;
%ignore Urho3D::M_MIN_NEARCLIP;
%csconst(1) Urho3D::M_MIN_NEARCLIP;
%constant float MMinNearclip = (float)0.00999999977;
%ignore Urho3D::M_MAX_FOV;
%csconst(1) Urho3D::M_MAX_FOV;
%constant float MMaxFov = (float)160;
%ignore Urho3D::M_LARGE_VALUE;
%csconst(1) Urho3D::M_LARGE_VALUE;
%constant float MLargeValue = (float)1.0E+8;
%constant float MInfinity = Urho3D::M_INFINITY;
%ignore Urho3D::M_INFINITY;
%constant float MDegtorad = Urho3D::M_DEGTORAD;
%ignore Urho3D::M_DEGTORAD;
%constant float MDegtorad2 = Urho3D::M_DEGTORAD_2;
%ignore Urho3D::M_DEGTORAD_2;
%constant float MRadtodeg = Urho3D::M_RADTODEG;
%ignore Urho3D::M_RADTODEG;
%ignore Urho3D::NUM_FRUSTUM_PLANES;
%csconst(1) Urho3D::NUM_FRUSTUM_PLANES;
%constant unsigned int NumFrustumPlanes = 6;
%ignore Urho3D::NUM_FRUSTUM_VERTICES;
%csconst(1) Urho3D::NUM_FRUSTUM_VERTICES;
%constant unsigned int NumFrustumVertices = 8;
%csconstvalue("0") Urho3D::PLANE_NEAR;
%csattribute(Urho3D::Vector2, %arg(bool), NaN, IsNaN);
%csattribute(Urho3D::Vector2, %arg(bool), Inf, IsInf);
%csattribute(Urho3D::Vector2, %arg(Urho3D::Vector2), OrthogonalClockwise, GetOrthogonalClockwise);
%csattribute(Urho3D::Vector2, %arg(Urho3D::Vector2), OrthogonalCounterClockwise, GetOrthogonalCounterClockwise);
%csattribute(Urho3D::Vector3, %arg(bool), NaN, IsNaN);
%csattribute(Urho3D::Vector3, %arg(bool), Inf, IsInf);
%csattribute(Urho3D::Vector4, %arg(bool), NaN, IsNaN);
%csattribute(Urho3D::Vector4, %arg(bool), Inf, IsInf);
%csattribute(Urho3D::Matrix3, %arg(bool), NaN, IsNaN);
%csattribute(Urho3D::Matrix3, %arg(bool), Inf, IsInf);
%csattribute(Urho3D::Quaternion, %arg(bool), NaN, IsNaN);
%csattribute(Urho3D::Quaternion, %arg(bool), Inf, IsInf);
%csattribute(Urho3D::Matrix4, %arg(bool), NaN, IsNaN);
%csattribute(Urho3D::Matrix4, %arg(bool), Inf, IsInf);
%csattribute(Urho3D::Matrix3x4, %arg(bool), NaN, IsNaN);
%csattribute(Urho3D::Matrix3x4, %arg(bool), Inf, IsInf);
%csattribute(Urho3D::AreaAllocator, %arg(int), Width, GetWidth);
%csattribute(Urho3D::AreaAllocator, %arg(int), Height, GetHeight);
%csattribute(Urho3D::AreaAllocator, %arg(bool), FastMode, GetFastMode);
%csattribute(Urho3D::Matrix2, %arg(bool), NaN, IsNaN);
%csattribute(Urho3D::Matrix2, %arg(bool), Inf, IsInf);
%csattribute(Urho3D::RandomEngine, %arg(unsigned int), UInt, GetUInt);
%csattribute(Urho3D::RandomEngine, %arg(double), Double, GetDouble);
%csattribute(Urho3D::RandomEngine, %arg(float), Float, GetFloat);
%csattribute(Urho3D::RandomEngine, %arg(ea::pair<float, float>), StandardNormalFloatPair, GetStandardNormalFloatPair);
%csattribute(Urho3D::RandomEngine, %arg(float), StandardNormalFloat, GetStandardNormalFloat);
%csattribute(Urho3D::RandomEngine, %arg(Urho3D::Vector2), DirectionVector2, GetDirectionVector2);
%csattribute(Urho3D::RandomEngine, %arg(Urho3D::Vector3), DirectionVector3, GetDirectionVector3);
%csattribute(Urho3D::RandomEngine, %arg(Urho3D::Quaternion), Quaternion, GetQuaternion);
%csattribute(Urho3D::SphericalHarmonicsDot9, %arg(Urho3D::Color), DebugColor, GetDebugColor);
%csattribute(Urho3D::TetrahedralMeshSurface, %arg(bool), ClosedSurface, IsClosedSurface);
