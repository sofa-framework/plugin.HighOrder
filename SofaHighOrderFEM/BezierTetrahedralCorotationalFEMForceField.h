
#ifndef SOFA_COMPONENT_FORCEFIELD_BEZIERTETRAHEDRALCOROTATIONALFEMFORCEFIELD_H
#define SOFA_COMPONENT_FORCEFIELD_BEZIERTETRAHEDRALCOROTATIONALFEMFORCEFIELD_H

#if !defined(__GNUC__) || (__GNUC__ > 3 || (_GNUC__ == 3 && __GNUC_MINOR__ > 3))
#pragma once
#endif
#include "initHighOrderFEM.h"
#include <sofa/core/behavior/ForceField.h>
#include <SofaBaseMechanics/MechanicalObject.h>
#include <sofa/helper/fixed_array.h>
#include <sofa/helper/vector.h>
#include <sofa/defaulttype/Vec.h>
#include <sofa/defaulttype/Mat.h>
#include <SofaBaseTopology/TopologyData.h>
#include <sofa/defaulttype/MatSym.h>

namespace sofa
{

namespace component
{
namespace topology
{
	template< class DataTypes> class HighOrderTetrahedronSetGeometryAlgorithms;
}
namespace forcefield
{

using namespace sofa::defaulttype;
using namespace sofa::component::topology;



template<class DataTypes>
class SOFA_HIGHORDER_FEM_API BezierTetrahedralCorotationalFEMForceField : public core::behavior::ForceField<DataTypes>
{
public:
    SOFA_CLASS(SOFA_TEMPLATE(BezierTetrahedralCorotationalFEMForceField,DataTypes), SOFA_TEMPLATE(core::behavior::ForceField,DataTypes));

    typedef core::behavior::ForceField<DataTypes> Inherited;
    typedef typename DataTypes::Real        Real        ;
    typedef typename DataTypes::Coord       Coord       ;
    typedef typename DataTypes::Deriv       Deriv       ;
    typedef typename DataTypes::VecCoord    VecCoord    ;
    typedef typename DataTypes::VecDeriv    VecDeriv    ;
    typedef typename DataTypes::VecReal     VecReal     ;
    typedef Data<VecCoord>                  DataVecCoord;
    typedef Data<VecDeriv>                  DataVecDeriv;    
	typedef sofa::core::topology::Topology::Tetrahedron Tetrahedron;
	typedef sofa::core::topology::Topology::TetraID TetraID;
	typedef sofa::core::topology::Topology::Tetra Tetra;
	typedef sofa::core::topology::Topology::Point Point;
	typedef sofa::core::topology::Topology::Triangle Triangle;
	typedef sofa::core::topology::Topology::Edge Edge;
	typedef sofa::core::topology::Topology::Quad Quad;
	typedef sofa::core::topology::Topology::Hexahedron Hexahedron;
	typedef sofa::core::topology::BaseMeshTopology::EdgesInTriangle EdgesInTriangle;
	typedef sofa::core::topology::BaseMeshTopology::EdgesInTetrahedron EdgesInTetrahedron;
	typedef sofa::core::topology::BaseMeshTopology::EdgesInQuad EdgesInQuad;
	typedef sofa::core::topology::BaseMeshTopology::EdgesInHexahedron EdgesInHexahedron;
	typedef sofa::core::topology::BaseMeshTopology::TrianglesInTetrahedron TrianglesInTetrahedron;
	typedef sofa::core::topology::BaseMeshTopology::TetrahedraAroundTriangle TetrahedraAroundTriangle;

	typedef helper::vector<Coord> SetAnisotropyDirectionArray; // When the model is anisotropic, for instance in invariant I4
	typedef helper::vector<Real> SetParameterArray; //necessary to store hyperelastic parameters (mu, lambda ...)

    typedef Mat<3,3,Real>       Mat3x3  ;
    typedef Mat<4,4,Real>       Mat4x4  ;
    typedef Mat<6,3,Real>       Mat6x3  ;
    typedef Mat<6,6,Real>     Mat6x6  ;
	typedef typename defaulttype::MatSym<3,Real> MatrixSym;
    // In case of non 3D template
    typedef Vec<3,Real> Vec3;
    typedef Vec<4,Real> Vec4;
    typedef StdVectorTypes< Vec3, Vec3, Real >     GeometricalTypes ; /// assumes the geometry object type is 3D
	typedef typename sofa::component::topology::HighOrderTetrahedronSetGeometryAlgorithms<GeometricalTypes>::VecPointID VecPointID;

    typedef helper::vector<Real> ParameterArray;
    typedef helper::vector<Coord> AnisotropyDirectionArray;

    typedef enum
    {
        POLAR_DECOMPOSITION,
        QR_DECOMPOSITION,
	    POLAR_DECOMPOSITION_MODIFIED,
		LINEAR_ELASTIC
    } RotationDecompositionMethod;

	/// the way the stiffness matrix should be computed on Bezier elements
	typedef enum 
	{
		AFFINE_ELEMENT_INTEGRATION=1,
		NUMERICAL_INTEGRATION=2,
		STANDARD_INTEGRATION=3
	} IntegrationMethod;

		/// the way the stiffness matrix should be computed on Bezier elements
	typedef enum 
	{
		ISOTROPIC=1,
		TRANSVERSE_ISOTROPIC=2,
		ORTHOTROPIC=3,
		CUBIC
	} ElasticitySymmetry;

protected:

	// the array where stiffness coefficients are stored for affine elements.
	std::vector<Mat4x4> affineStiffnessCoefficientArray;
	// the data stored for each integration point
	struct NumericalIntegrationStiffnessData {
		// the weight of the integration point
		Real integrationWeight;
		// for each pair of control point store the weight matrix 6 w_\gamma * dN_p/d\param_i(\param_gamma) *  dN_q/d\param_j (\param_gamma)
		std::vector<Mat4x4> weightArray;
		// for each control point  store the derivative of the shape functions
		std::vector<Deriv> coefficientArray;
		/// barycentric coordinate of the integration point \param_i
		Vec4 integrationPoint;
	};
	// the array where the weights and coordinates of each integration points are stored
	std::vector<NumericalIntegrationStiffnessData> numericalIntegrationStiffnessDataArray;
	/// the elasticity tensor 
	Mat6x6 elasticityTensor;

    /// data structure stored for each tetrahedron
    class TetrahedronRestInformation
	{
	public:
		typedef typename DataTypes::Real  Real;
		typedef Mat<3,3,Real> Mat3x3;
		

		/// rest volume
      
		Coord shapeVector[4];
        Coord restEdgeVector[6];
        Mat3x3 rotation; // rotation from deformed to rest configuration
        Mat3x3 restRotation; // used for QR decomposition
		helper::vector<Mat3x3> stiffnessVector; // the nc*(nc+1)/2 stiffness matrices where nc is the number of control points
		size_t v[4]; // the indices of the 4 vertices
		helper::vector<Mat3x3> rotatedStiffnessVector; // the nc*(nc+1)/2 stiffness matrices where nc is the number of control points
		helper::vector<Mat3x3> reducedStiffnessVector;
		// store 6 2x3 matrices per integration points
		helper::vector<  helper::vector<Mat3x3> >  integrationPointsStiffnessVector;
		// store the 6 rest edge vector for each integration point
		helper::vector< helper::vector<Coord> > integrationPointsRestEdgeVector;
		// store a rest rotation for each integration point
		helper::vector< Mat3x3 > integrationPointsRestRotationArray;

        /// Output stream
        inline friend std::ostream& operator<< ( std::ostream& os, const TetrahedronRestInformation& /*eri*/ )
        {
            return os;
        }

        /// Input stream
        inline friend std::istream& operator>> ( std::istream& in, TetrahedronRestInformation& /*eri*/ )
        {
            return in;
        }

        TetrahedronRestInformation()
        {
        }
    };

    class FTCFTetrahedronHandler : public TopologyDataHandler<Tetrahedron, sofa::helper::vector<TetrahedronRestInformation> >
    {
    public:
        typedef typename BezierTetrahedralCorotationalFEMForceField<DataTypes>::TetrahedronRestInformation TetrahedronRestInformation;

        FTCFTetrahedronHandler(BezierTetrahedralCorotationalFEMForceField<DataTypes>* ff,
                TetrahedronData<sofa::helper::vector<TetrahedronRestInformation> >* data )
            :TopologyDataHandler<Tetrahedron, sofa::helper::vector<TetrahedronRestInformation> >(data)
            ,ff(ff)
        {

        }

        void applyCreateFunction(unsigned int, TetrahedronRestInformation &t, const Tetrahedron
                &, const sofa::helper::vector<unsigned int> &, const sofa::helper::vector<double> &);

    protected:
        BezierTetrahedralCorotationalFEMForceField<DataTypes>* ff;

    };

    TetrahedronData<sofa::helper::vector<TetrahedronRestInformation> > tetrahedronInfo;


    sofa::core::topology::BaseMeshTopology* _topology;
	sofa::component::topology::HighOrderTetrahedronSetGeometryAlgorithms<GeometricalTypes>* bezierTetraGeo;
    VecCoord  _initialPoints;///< the intial positions of the points

    bool updateMatrix;
    bool updateTopologyInfo;

    Data<std::string> d_method; ///< the computation method of the displacements
    RotationDecompositionMethod decompositionMethod;

    Data<std::string> d_anisotropy; // the type of isotropy
	ElasticitySymmetry elasticitySymmetry;

    Data<Real> d_poissonRatio; // stiffness coefficient for isotropic elasticity;
    Data<Real> d_youngModulus;

	Data<ParameterArray> d_anisotropyParameter;
	Data<AnisotropyDirectionArray> d_anisotropyDirection;

	std::vector<Mat3x3> anisotropyMatrixArray;
	std::vector<Real> anisotropyScalarArray;


    Real lambda;  /// first Lame coefficient
    Real mu;    /// second Lame coefficient

	/// the order of integration for numerical integration
	Data<size_t>	     numericalIntegrationOrder;
	/// the type of numerical integration method chosen
	Data<size_t>	     numericalIntegrationMethod;
	/// the type of integration method chosen for non linear element.
	Data<std::string>	 d_integrationMethod; 
	IntegrationMethod    integrationMethod;
	Data<bool> d_oneRotationPerIntegrationPoint;


    BezierTetrahedralCorotationalFEMForceField();

    virtual ~BezierTetrahedralCorotationalFEMForceField();

public:

    virtual void init();


    virtual void addForce(const sofa::core::MechanicalParams* /*mparams*/ /* PARAMS FIRST */, DataVecDeriv &  dataF, const DataVecCoord &  dataX , const DataVecDeriv & dataV ) ;
    virtual void addDForce(const sofa::core::MechanicalParams* /*mparams*/ /* PARAMS FIRST */, DataVecDeriv&   datadF , const DataVecDeriv&   datadX ) ;
    virtual SReal getPotentialEnergy(const core::MechanicalParams*, const DataVecCoord&) const;

    void updateTopologyInformation();

    virtual Real getLambda() const { return lambda;}
    virtual Real getMu() const { return mu;}

    void setYoungModulus(const double modulus)
    {
        d_youngModulus.setValue((Real)modulus);
    }
    void setPoissonRatio(const double ratio)
    {
        d_poissonRatio.setValue((Real)ratio);
    }
    void setRotationDecompositionMethod( const RotationDecompositionMethod m)
    {
        decompositionMethod=m;
    }
    void draw(const core::visual::VisualParams* vparams);
    /// compute lambda and mu based on the Young modulus and Poisson ratio
    void updateLameCoefficients();

	void computeTetrahedronStiffnessEdgeMatrix(const Coord position[4],Mat3x3 edgeStiffness[6]);

	friend class FTCFTetrahedronHandler;

protected :
    FTCFTetrahedronHandler* tetrahedronHandler;

	static void computeQRRotation( Mat3x3 &r, const Coord *dp);

	virtual const helper::vector<Mat3x3> &getStiffnessArray(const size_t i,
		TetrahedronRestInformation *restTetra);

	virtual const helper::vector<Mat3x3> &getRotatedStiffnessArray(const size_t i,
		const TetrahedronRestInformation *restTetra);
	// update anisotropyMatrixArray and anisotropyScalarArray based on input data
	void assembleAnisotropicTensors();
	// compute elasticity tensor for isotropic and anisotropic cases
	void computeElasticityTensor(); 		

};


} // namespace forcefield

} // namespace component

} // namespace sofa

#endif // SOFA_COMPONENT_FORCEFIELD_FASTTETRAHEDRALCOROTATIONALFORCEFIELD_H
