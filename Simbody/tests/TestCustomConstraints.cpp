/* -------------------------------------------------------------------------- *
 *                      SimTK Core: SimTK Simbody(tm)                         *
 * -------------------------------------------------------------------------- *
 * This is part of the SimTK Core biosimulation toolkit originating from      *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org.               *
 *                                                                            *
 * Portions copyright (c) 2008 Stanford University and the Authors.           *
 * Authors: Peter Eastman                                                     *
 * Contributors:                                                              *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,    *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR      *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE  *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                     *
 * -------------------------------------------------------------------------- */

#include "SimTKsimbody.h"
#include "SimTKcommon/Testing.h"

using namespace SimTK;
using namespace std;

const int NUM_BODIES = 10;
const Real BOND_LENGTH = 0.5;
//const Real TOL = 1e-10;
//
//#define ASSERT(cond) {SimTK_ASSERT_ALWAYS(cond, "Assertion failed");}
//
//template <class T>
//void assertEqual(T val1, T val2, double tol = TOL) {
//    ASSERT(abs(val1-val2) < tol);
//}
//
//template <int N>
//void assertEqual(Vec<N> val1, Vec<N> val2, double tol) {
//    for (int i = 0; i < N; ++i)
//        ASSERT(abs(val1[i]-val2[i]) < tol);
//}
//
//template<>
//void assertEqual(SpatialVec val1, SpatialVec val2, double tol) {
//    assertEqual(val1[0], val2[0], tol);
//    assertEqual(val1[1], val2[1], tol);
//}
//
//template <>
//void assertEqual(Vector val1, Vector val2, double tol) {
//    ASSERT(val1.size() == val2.size());
//    for (int i = 0; i < val1.size(); ++i)
//        ASSERT(abs(val1[i]-val2[i]) < tol);
//}

/* This Measure returns the instantaneous power being generated by the
indicated Constraint */
template <class T>
class PowerMeasure : public Measure_<T> {
public:
    SimTK_MEASURE_HANDLE_PREAMBLE(PowerMeasure, Measure_<T>);

    PowerMeasure(Subsystem& sub,
                 const Constraint& constraint)
    :   Measure_<T>(sub, new Implementation(constraint), SetHandle()) {}
    SimTK_MEASURE_HANDLE_POSTSCRIPT(PowerMeasure, Measure_<T>);
};


template <class T>
class PowerMeasure<T>::Implementation : public Measure_<T>::Implementation {
public:
    Implementation(const Constraint& constraint) 
    :   Measure_<T>::Implementation(1), m_constraint(constraint) {}

    // Default copy constructor, destructor, copy assignment are fine.

    // Implementations of virtual methods.
    Implementation* cloneVirtual() const {return new Implementation(*this);}
    int getNumTimeDerivativesVirtual() const {return 0;}
    Stage getDependsOnStageVirtual(int order) const 
    {   return Stage::Acceleration; }

    void calcCachedValueVirtual(const State& s, int derivOrder, T& value) const
    {
        SimTK_ASSERT1_ALWAYS(derivOrder==0,
            "PowerMeasure::Implementation::calcCachedValueVirtual():"
            " derivOrder %d seen but only 0 allowed.", derivOrder);

        value = m_constraint.calcPower(s);
    }
private:
    const Constraint m_constraint;
};

/**
 * A Function that takes a single argument and returns it.
 */

class LinearFunction : public Function {
public:
    Real calcValue(const Vector& x) const {
        return x[0];
    }
    Real calcDerivative(const Array_<int>& derivComponents, const Vector& x) const {
        if (derivComponents.size() == 1)
            return 1;
        return 0;
    }
    int getArgumentSize() const {
        return 1;
    }
    int getMaxDerivativeOrder() const {
        return 100;
    }
};

/**
 * A Function that relates three different arguments.
 */

class CompoundFunction : public Function {
public:
    Real calcValue(const Vector& x) const {
        return 1*x[0]+2*x[1]+3*x[2];
    }
    Real calcDerivative(const Array_<int>& derivComponents, const Vector& x) const {
        if (derivComponents.size() == 1) {
            return derivComponents[0]+1; // i.e. coef. 1, 2, or 3
        }
        return 0;
    }
    int getArgumentSize() const {
        return 3;
    }
    int getMaxDerivativeOrder() const {
        return 2;
    }
};

/**
 * Create a system consisting of a chain of Gimbal joints.
 */

void createGimbalSystem(MultibodySystem& system) {
    SimbodyMatterSubsystem& matter = system.updMatterSubsystem();
    GeneralForceSubsystem forces(system);
    Force::UniformGravity gravity(forces, matter, Vec3(0, -1, 0), 0);
    Body::Rigid body(MassProperties(1.0, Vec3(0), Inertia(1)));
    for (int i = 0; i < NUM_BODIES; ++i) {
        MobilizedBody& parent = matter.updMobilizedBody(MobilizedBodyIndex(matter.getNumBodies()-1));
        MobilizedBody::Gimbal b(parent, Transform(Vec3(0)), body, Transform(Vec3(BOND_LENGTH, 0, 0)));
    }
}

/**
 * Create a system consisting of a chain of Ball joints.
 */

void createBallSystem(MultibodySystem& system) {
    SimbodyMatterSubsystem& matter = system.updMatterSubsystem();
    GeneralForceSubsystem forces(system);
    Force::UniformGravity gravity(forces, matter, Vec3(0, -1, 0), 0);
    Body::Rigid body(MassProperties(1.0, Vec3(0), Inertia(1)));
    for (int i = 0; i < NUM_BODIES; ++i) {
        MobilizedBody& parent = matter.updMobilizedBody(MobilizedBodyIndex(matter.getNumBodies()-1));
        MobilizedBody::Ball b(parent, Transform(Vec3(0)), body, Transform(Vec3(BOND_LENGTH, 0, 0)));
    }
}

/**
 * Create a system consisting of a chain of Planar joints.
 */

void createPlanarSystem(MultibodySystem& system) {
    SimbodyMatterSubsystem& matter = system.updMatterSubsystem();
    GeneralForceSubsystem forces(system);
    Force::UniformGravity gravity(forces, matter, Vec3(0, -1, 0), 0);
    Body::Rigid body(MassProperties(1.0, Vec3(0), Inertia(1)));
    for (int i = 0; i < NUM_BODIES; ++i) {
        MobilizedBody& parent = matter.updMobilizedBody(MobilizedBodyIndex(matter.getNumBodies()-1));
        MobilizedBody::Planar b(parent, Transform(Vec3(0)), body, Transform(Vec3(BOND_LENGTH, 0, 0)));
    }
}

/**
 * Create a system consisting of a chain of Cylinder joints.
 */

void createCylinderSystem(MultibodySystem& system) {
    SimbodyMatterSubsystem& matter = system.updMatterSubsystem();
    GeneralForceSubsystem forces(system);
    // Skew gravity so moving takes work.
    Force::UniformGravity gravity(forces, matter, Vec3(0, -2, -3));
    for (int i = 0; i < NUM_BODIES; ++i) {
        MobilizedBody& parent = 
            matter.updMobilizedBody(MobilizedBodyIndex(matter.getNumBodies()-1));
        const Real mass = 1 + 0.1*i;
        Body::Rigid body(MassProperties(mass, Vec3(0), mass*UnitInertia(1)));
        MobilizedBody::Cylinder b(parent, Transform(Vec3(.1,.2,.3)), 
                                  body, Transform(Vec3(BOND_LENGTH, 0, 0)));
    }
}

/**
 * Create a random state for the system.
 */

void createState(MultibodySystem& system, State& state, const Vector& y=Vector()) {
    system.realizeTopology();
    state = system.getDefaultState();
    if (y.size() > 0)
        state.updY() = y;
    else {
        Random::Uniform random;
        for (int i = 0; i < state.getNY(); ++i)
            state.updY()[i] = random.getValue();
    }
    system.realize(state, Stage::Velocity);
    Vector dummy; // no error projection to do
    // Solve to tight tolerance here
    system.project(state, 1e-12, Vector(state.getNY(), 1), 
                   Vector(state.getNYErr(), 1), dummy);
    system.realize(state, Stage::Acceleration);
}

void testCoordinateCoupler1() {

    // Create a system using three CoordinateCouplers to fix the orientation 
    // of one body.
    
    MultibodySystem system1;
    SimbodyMatterSubsystem matter1(system1);
    createGimbalSystem(system1);
    MobilizedBody& first = matter1.updMobilizedBody(MobilizedBodyIndex(1));
    std::vector<MobilizedBodyIndex> bodies(1);
    std::vector<MobilizerQIndex> coordinates(1);
    bodies[0] = MobilizedBodyIndex(1);
    coordinates[0] = MobilizerQIndex(0);
    Constraint::CoordinateCoupler coupler1(matter1, new LinearFunction(), bodies, coordinates);
    coordinates[0] = MobilizerQIndex(1);
    Constraint::CoordinateCoupler coupler2(matter1, new LinearFunction(), bodies, coordinates);
    coordinates[0] = MobilizerQIndex(2);
    Constraint::CoordinateCoupler coupler3(matter1, new LinearFunction(), bodies, coordinates);
    State state1;
    createState(system1, state1);

    // Create a system using a ConstantOrientation constraint to do the 
    // same thing.
    
    MultibodySystem system2;
    SimbodyMatterSubsystem matter2(system2);
    createGimbalSystem(system2);
    Constraint::ConstantOrientation orient(matter2.updGround(), Rotation(), 
        matter2.updMobilizedBody(MobilizedBodyIndex(1)), Rotation());
    State state2;
    createState(system2, state2, state1.getY());
    
    // Compare the results.
    
    SimTK_TEST_EQ(state1.getQ(), state2.getQ());
    SimTK_TEST_EQ(state1.getQDot(), state2.getQDot());
    SimTK_TEST_EQ(state1.getQDotDot(), state2.getQDotDot());
    SimTK_TEST_EQ(state1.getU(), state2.getU());
    SimTK_TEST_EQ(state1.getUDot(), state2.getUDot());
}

void testCoordinateCoupler2() {
    
    // Create a system involving a constraint that affects multiple mobilizers.
    
    MultibodySystem system;
    SimbodyMatterSubsystem matter(system);
    createCylinderSystem(system);
    MobilizedBody& first = matter.updMobilizedBody(MobilizedBodyIndex(1));
    std::vector<MobilizedBodyIndex> mobilizers(3);
    std::vector<MobilizerQIndex>    coordinates(3);
    mobilizers[0]  = MobilizedBodyIndex(1);
    mobilizers[1]  = MobilizedBodyIndex(1);
    mobilizers[2]  = MobilizedBodyIndex(5);
    coordinates[0] = MobilizerQIndex(0);
    coordinates[1] = MobilizerQIndex(1);
    coordinates[2] = MobilizerQIndex(1);
    Function* function = new CompoundFunction();
    Constraint::CoordinateCoupler coupler(matter, function, 
                                          mobilizers, coordinates);
    State state;
    createState(system, state);
    
    // Make sure the constraint is satisfied.
    
    Vector cq(function->getArgumentSize());
    for (int i = 0; i < cq.size(); ++i)
        cq[i] = matter.getMobilizedBody(mobilizers[i])
                      .getOneQ(state, coordinates[i]);
    SimTK_TEST_EQ(0.0, function->calcValue(cq));
    
    // Simulate it and make sure the constraint is working correctly and
    // energy is being conserved. This is a workless constraint so the
    // power should be zer
    system.realize(state, Stage::Acceleration);
    Real energy0 = system.calcEnergy(state);

    RungeKuttaMersonIntegrator integ(system);
    integ.setReturnEveryInternalStep(true);
    integ.initialize(state);
    while (integ.getTime() < 10.0) {
        integ.stepTo(10.0);
        const State& istate = integ.getState();
        system.realize(istate, Stage::Acceleration);
        const Vector& u = istate.getU();
        const Real energy = system.calcEnergy(istate);
        const Real power  = coupler.calcPower(istate);


        for (int i = 0; i < cq.size(); ++i)
            cq[i] = matter.getMobilizedBody(mobilizers[i])
                          .getOneQ(istate, coordinates[i]);
        SimTK_TEST_EQ_TOL(0.0, function->calcValue(cq), 
                          integ.getConstraintToleranceInUse());

        // Power output should always be zero to machine precision.
        SimTK_TEST_EQ(0.0, power);

        // Energy conservation depends on global integration accuracy;
        // accuracy returned here is local so we'll fudge at 10X.
        const Real etol = 10*integ.getAccuracyInUse()
                          *std::max(std::abs(energy), std::abs(energy0));
        SimTK_TEST_EQ_TOL(energy0, energy, etol);
    }
}

void testCoordinateCoupler3() {
    
    // Create a system involving a constrained body for which qdot != u.
    
    MultibodySystem system;
    SimbodyMatterSubsystem matter(system);
    createBallSystem(system);
    MobilizedBody& first = matter.updMobilizedBody(MobilizedBodyIndex(1));
    std::vector<MobilizedBodyIndex> bodies(3);
    std::vector<MobilizerQIndex> coordinates(3);
    bodies[0] = MobilizedBodyIndex(1);
    bodies[1] = MobilizedBodyIndex(1);
    bodies[2] = MobilizedBodyIndex(1);
    coordinates[0] = MobilizerQIndex(0);
    coordinates[1] = MobilizerQIndex(1);
    coordinates[2] = MobilizerQIndex(2);
    Function* function = new CompoundFunction();
    Constraint::CoordinateCoupler coupler(matter, function, bodies, coordinates);
    State state;
    createState(system, state);
    
    // Make sure the constraint is satisfied.
    
    Vector args(function->getArgumentSize());
    for (int i = 0; i < args.size(); ++i)
        args[i] = matter.getMobilizedBody(bodies[i]).getOneQ(state, coordinates[i]);
    SimTK_TEST_EQ(0.0, function->calcValue(args));
    
    // Simulate it and make sure the constraint is working correctly and 
    // energy is being conserved.
    
    const Real energy0 = system.calcEnergy(state);
    RungeKuttaMersonIntegrator integ(system);
    integ.setReturnEveryInternalStep(true);
    integ.initialize(state);
    while (integ.getTime() < 10.0) {
        integ.stepTo(10.0);
        const State& istate = integ.getState();
        const Real energy = system.calcEnergy(istate);

        for (int i = 0; i < args.size(); ++i)
            args[i] = matter.getMobilizedBody(bodies[i])
                            .getOneQ(integ.getState(), coordinates[i]);
        // Constraints are applied to unnormalized quaternions. When they are 
        // normalized, that can increase the constraint error. That is why we 
        // need the factor of 3 in the next line.
        // TODO: Huh? (sherm)
        SimTK_TEST_EQ_TOL(0.0, function->calcValue(args), 
                          3*integ.getConstraintToleranceInUse());
        
         // Energy conservation depends on global integration accuracy;
        // accuracy returned here is local so we'll fudge at 10X.
        const Real etol = 10*integ.getAccuracyInUse()
                          *std::max(std::abs(energy), std::abs(energy0));        
        SimTK_TEST_EQ_TOL(energy0, energy, etol);       
    }
}

void testSpeedCoupler1() {

    // Create a system using a SpeedCoupler to fix one speed.
    
    MultibodySystem system1;
    SimbodyMatterSubsystem matter1(system1);
    createGimbalSystem(system1);
    MobilizedBody& first = matter1.updMobilizedBody(MobilizedBodyIndex(1));
    std::vector<MobilizedBodyIndex> bodies(1);
    std::vector<MobilizerUIndex> speeds(1);
    bodies[0] = MobilizedBodyIndex(1);
    speeds[0] = MobilizerUIndex(2);
    Constraint::SpeedCoupler coupler1(matter1, new LinearFunction(), bodies, speeds);
    State state1;
    createState(system1, state1);

    // Create a system using a ConstantSpeed constraint to do the same thing.
    
    MultibodySystem system2;
    SimbodyMatterSubsystem matter2(system2);
    createGimbalSystem(system2);
    Constraint::ConstantSpeed orient(matter2.updMobilizedBody(MobilizedBodyIndex(1)), MobilizerUIndex(2), 0);
    State state2;
    createState(system2, state2, state1.getY());
    
    // Compare the results.
    
    SimTK_TEST_EQ(state1.getQ(), state2.getQ());
    SimTK_TEST_EQ(state1.getQDot(), state2.getQDot());
    SimTK_TEST_EQ(state1.getQDotDot(), state2.getQDotDot());
    SimTK_TEST_EQ(state1.getU(), state2.getU());
    SimTK_TEST_EQ(state1.getUDot(), state2.getUDot());
}

void testSpeedCoupler2() {
    
    // Create a system involving a constraint that affects three different 
    // bodies.
    
    MultibodySystem system;
    SimbodyMatterSubsystem matter(system);
    createGimbalSystem(system);
    MobilizedBody& first = matter.updMobilizedBody(MobilizedBodyIndex(1));
    std::vector<MobilizedBodyIndex> bodies(3);
    std::vector<MobilizerUIndex> speeds(3);
    bodies[0] = MobilizedBodyIndex(1);
    bodies[1] = MobilizedBodyIndex(3);
    bodies[2] = MobilizedBodyIndex(5);
    speeds[0] = MobilizerUIndex(0);
    speeds[1] = MobilizerUIndex(0);
    speeds[2] = MobilizerUIndex(1);
    Function* function = new CompoundFunction();
    Constraint::SpeedCoupler coupler(matter, function, bodies, speeds);
    State state;
    createState(system, state);
    
    // Make sure the constraint is satisfied.
    
    Vector args(function->getArgumentSize());
    for (int i = 0; i < args.size(); ++i)
        args[i] = matter.getMobilizedBody(bodies[i]).getOneU(state, speeds[i]);
    SimTK_TEST_EQ(0.0, function->calcValue(args));
    
    // Simulate it and make sure the constraint is working correctly and 
    // energy is being conserved. This should be workless and power should
    // always be zero.
    
    Real energy0 = system.calcEnergy(state);
    RungeKuttaMersonIntegrator integ(system);
    integ.setReturnEveryInternalStep(true);
    integ.initialize(state);
    while (integ.getTime() < 10.0) {
        integ.stepTo(10.0);
        const State& istate = integ.getState();
        system.realize(istate, Stage::Acceleration);
        const Real energy = system.calcEnergy(istate);
        const Real power = coupler.calcPower(istate);

        for (int i = 0; i < args.size(); ++i)
            args[i] = matter.getMobilizedBody(bodies[i]).getOneU(istate, speeds[i]);
        SimTK_TEST_EQ_TOL(0.0, function->calcValue(args), 
                          integ.getConstraintToleranceInUse());
        
        SimTK_TEST_EQ_TOL(0.0, power, 10*SignificantReal);

        // Energy conservation depends on global integration accuracy;
        // accuracy returned here is local so we'll fudge at 10X.
        const Real etol = 10*integ.getAccuracyInUse()
                          *std::max(std::abs(energy), std::abs(energy0));        
        SimTK_TEST_EQ_TOL(energy0, energy, etol);
    }
}

void testSpeedCoupler3() {
    
    // Create a system with a constraint that uses both u's and q's.
    // This will not be workless in general.
    
    MultibodySystem system;
    SimbodyMatterSubsystem matter(system);
    createCylinderSystem(system);
    MobilizedBody& first = matter.updMobilizedBody(MobilizedBodyIndex(1));
    std::vector<MobilizedBodyIndex> ubody(2), qbody(1);
    std::vector<MobilizerUIndex> uindex(2);
    std::vector<MobilizerQIndex> qindex(1);
    ubody[0] = MobilizedBodyIndex(1);
    ubody[1] = MobilizedBodyIndex(3);
    qbody[0] = MobilizedBodyIndex(5);
    uindex[0] = MobilizerUIndex(0);
    uindex[1] = MobilizerUIndex(1);
    qindex[0] = MobilizerQIndex(1);
    Function* function = new CompoundFunction();
    Constraint::SpeedCoupler coupler(matter, function, ubody, uindex, 
                                     qbody, qindex);
    PowerMeasure<Real> powMeas(matter, coupler);
    Measure::Zero zeroMeas(matter);
    Measure::Integrate workMeas(matter, powMeas, zeroMeas); 

    State state;
    createState(system, state);
    workMeas.setValue(state, 0); // override createState
    
    // Make sure the constraint is satisfied.
    
    Vector args(function->getArgumentSize());
    args[0] = matter.getMobilizedBody(ubody[0]).getOneU(state, uindex[0]);
    args[1] = matter.getMobilizedBody(ubody[1]).getOneU(state, uindex[1]);
    args[2] = matter.getMobilizedBody(qbody[0]).getOneQ(state, qindex[0]);
    SimTK_TEST_EQ(0.0, function->calcValue(args));
    
    // Simulate it and make sure the constraint is working correctly.
    // We don't expect energy to be conserved here but energy minus the
    // work done by the constraint should be conserved.
    Real energy0 = system.calcEnergy(state);

    RungeKuttaMersonIntegrator integ(system);
    integ.setAccuracy(1e-6);
    integ.setReturnEveryInternalStep(true);
    integ.initialize(state);

    while (integ.getTime() < 10.0) {
        integ.stepTo(10.0);
        const State& istate = integ.getState();
        system.realize(istate, Stage::Acceleration);
        const Real energy = system.calcEnergy(istate);
        const Real power = powMeas.getValue(istate);
        const Real work =  workMeas.getValue(istate);

        args[0] = matter.getMobilizedBody(ubody[0]).getOneU(state, uindex[0]);
        args[1] = matter.getMobilizedBody(ubody[1]).getOneU(state, uindex[1]);
        args[2] = matter.getMobilizedBody(qbody[0]).getOneQ(state, qindex[0]);
        SimTK_TEST_EQ_TOL(0.0, function->calcValue(args), 
                          integ.getConstraintToleranceInUse());

        // Energy conservation depends on global integration accuracy;
        // accuracy returned here is local so we'll fudge at 10X.
        const Real etol = 10*integ.getAccuracyInUse()
                          *std::max(std::abs(energy-work), std::abs(energy0));        
        SimTK_TEST_EQ_TOL(energy0, energy-work, etol)

    }
}

void testPrescribedMotion1() {
    
    // Create a system requiring simple linear motion of one Q. This
    // may require that the constraint do work.
    // (The way the cylinder system is structured it only takes work to
    // keep body one at a uniform velocity; the rest are in free fall.)
    
    MultibodySystem system;
    SimbodyMatterSubsystem matter(system);
    createCylinderSystem(system);
    MobilizedBodyIndex body = MobilizedBodyIndex(1);
    MobilizerQIndex coordinate = MobilizerQIndex(1);
    Vector coefficients(2);
    coefficients[0] = 0.1;
    coefficients[1] = 0.0;
    Function* function = new Function::Linear(coefficients);
    Constraint::PrescribedMotion constraint(matter, function, body, coordinate);
    PowerMeasure<Real> powMeas(matter, constraint);
    Measure::Zero zeroMeas(matter);
    Measure::Integrate workMeas(matter, powMeas, zeroMeas);     
    
    State state;
    createState(system, state);
    workMeas.setValue(state, 0); // override createState
    
    // Make sure the constraint is satisfied.
    
    Vector args(1, state.getTime());
    SimTK_TEST_EQ(function->calcValue(args), 
                  matter.getMobilizedBody(body).getOneQ(state, coordinate));
    
    // Simulate it and make sure the constraint is working correctly.
    const Real energy0 = system.calcEnergy(state);   
    RungeKuttaMersonIntegrator integ(system);
    integ.setReturnEveryInternalStep(true);
    integ.initialize(state);
    while (integ.getTime() < 10.0) {
        integ.stepTo(10.0);
        const State& istate = integ.getState();
        system.realize(istate, Stage::Acceleration);
        const Real energy = system.calcEnergy(istate);
        const Real power = powMeas.getValue(istate);
        const Real work =  workMeas.getValue(istate);

        Vector args(1, istate.getTime());
        const Real q = matter.getMobilizedBody(body).getOneQ(istate, coordinate);
        SimTK_TEST_EQ_TOL(function->calcValue(args), q, 
                          integ.getConstraintToleranceInUse());

        // Energy conservation depends on global integration accuracy;
        // accuracy returned here is local so we'll fudge at 10X.
        const Real etol = 10*integ.getAccuracyInUse()
                          *std::max(std::abs(energy-work), std::abs(energy0));        
        SimTK_TEST_EQ_TOL(energy0, energy-work, etol)
    }
}

void testPrescribedMotion2() {
    
    // Create a system prescribing the motion of two Qs.
    
    MultibodySystem system;
    SimbodyMatterSubsystem matter(system);
    createCylinderSystem(system);
    MobilizedBodyIndex body1 = MobilizedBodyIndex(2);
    MobilizerQIndex coordinate1 = MobilizerQIndex(1);
    Vector coefficients1(2);
    coefficients1[0] = 0.1;
    coefficients1[1] = 0.0;
    Function* function1 = new Function::Linear(coefficients1);
    Constraint::PrescribedMotion constraint1(matter, function1, body1, coordinate1);
    MobilizedBodyIndex body2 = MobilizedBodyIndex(2);
    MobilizerQIndex coordinate2 = MobilizerQIndex(0);
    Vector coefficients2(3);
    coefficients2[0] = 0.5;
    coefficients2[1] = -0.2;
    coefficients2[2] = 1.1;
    Function* function2 = new Function::Polynomial(coefficients2);
    Constraint::PrescribedMotion constraint2(matter, function2, body2, coordinate2);
    
    // Must track work done by the constraints in order to check that
    // energy is conserved.
    Measure::Zero zeroMeas(matter);
    PowerMeasure<Real> powMeas1(matter, constraint1);
    Measure::Integrate workMeas1(matter, powMeas1, zeroMeas);     
    PowerMeasure<Real> powMeas2(matter, constraint2);
    Measure::Integrate workMeas2(matter, powMeas2, zeroMeas);    
    
    State state;
    createState(system, state);
    workMeas1.setValue(state, 0); // override createState
    workMeas2.setValue(state, 0); // override createState
    
    // Make sure the constraint is satisfied.
    
    Vector args(1, state.getTime());
    SimTK_TEST_EQ(function1->calcValue(args), 
        matter.getMobilizedBody(body1).getOneQ(state, coordinate1));
    SimTK_TEST_EQ(function2->calcValue(args), 
        matter.getMobilizedBody(body2).getOneQ(state, coordinate2));
    
    // Simulate it and make sure the constraint is working correctly and energy is being conserved.
    const Real energy0 = system.calcEnergy(state);   
    
    RungeKuttaMersonIntegrator integ(system);
    integ.setReturnEveryInternalStep(true);
    integ.initialize(state);
    while (integ.getTime() < 10.0) {
        integ.stepTo(10.0);
        const State& istate = integ.getState();
        system.realize(istate, Stage::Acceleration);
        const Real energy = system.calcEnergy(istate);
        const Real power1 = powMeas1.getValue(istate);
        const Real work1 =  workMeas1.getValue(istate);
        const Real power2 = powMeas2.getValue(istate);
        const Real work2 =  workMeas2.getValue(istate);

        Vector args(1, istate.getTime());
        SimTK_TEST_EQ_TOL(function1->calcValue(args), 
            matter.getMobilizedBody(body1).getOneQ(istate, coordinate1), 
            integ.getConstraintToleranceInUse());
        SimTK_TEST_EQ_TOL(function2->calcValue(args), 
            matter.getMobilizedBody(body2).getOneQ(istate, coordinate2), 
            integ.getConstraintToleranceInUse());

        // Energy conservation depends on global integration accuracy;
        // accuracy returned here is local so we'll fudge at 10X.
        const Real etol = 10*integ.getAccuracyInUse()
                          *std::max(std::abs(energy-(work1+work2)), std::abs(energy0));        
        SimTK_TEST_EQ_TOL(energy0, energy-(work1+work2), etol)
    }
}

int main() {
    SimTK_START_TEST("TestCustomConstraints");
        SimTK_SUBTEST(testCoordinateCoupler1);
        SimTK_SUBTEST(testCoordinateCoupler2);
        SimTK_SUBTEST(testCoordinateCoupler3);
        SimTK_SUBTEST(testSpeedCoupler1);
        SimTK_SUBTEST(testSpeedCoupler2);
        SimTK_SUBTEST(testSpeedCoupler3);
        SimTK_SUBTEST(testPrescribedMotion1);
        SimTK_SUBTEST(testPrescribedMotion2);
    SimTK_END_TEST();
}
