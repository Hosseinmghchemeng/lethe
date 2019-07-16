//BASE
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/base/function.h>
#include <deal.II/base/utilities.h>
#include <deal.II/base/index_set.h>

//NUMERICS
#include <deal.II/numerics/vector_tools.h>
#include <deal.II/numerics/matrix_tools.h>

//GRID
#include <deal.II/grid/tria.h>
#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/manifold_lib.h>

//LACS
#include <deal.II/lac/trilinos_vector.h>
#include <deal.II/lac/full_matrix.h>
#include <deal.II/lac/sparse_matrix.h>
#include <deal.II/lac/dynamic_sparsity_pattern.h>
#include <deal.II/lac/solver_cg.h>
#include <deal.II/lac/precondition.h>


//DOFS
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_renumbering.h>
#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/dofs/dof_tools.h>


// Distributed
#include <deal.II/distributed/tria.h>

#include "ibcomposer.h"
#include "iblevelsetfunctions.h"
#include "write_data.h"
#include "../tests.h"

// Mes ajouts so far
#include "nouvtriangles.h"
#include "area.h"


using namespace dealii;
void test0_nouv_tri()
{
    // asserts that "nouv_triangles" works well
    Point<2> pt1 (0,0);
    Point<2> pt2 (1,0);
    Point<2> pt3 (0,1);
    Point<2> pt4 (1,1);
    Vector<Point<2>> coor(4);
    coor[0] = pt1;
    coor[1] = pt2;
    coor[2] = pt3;
    coor[3] = pt4;

    Vector<double> distance1  {1,1,1,-1};
    Vector<double> distance2  {1,-1,-1,-1};
    Vector<double> distance3  {1,1,-1,-1};

    Vector<Point<2>> decomp_elem(9);
    std::vector<int>    corresp(9);
    std::vector<Point<2>>    num_elem(6);
    Vector<node_status> No_pts_solid(4);
    int nb_poly;

    Vector<int> cor_thq1  {2, 0, 5, 0, 4, 5, 1, 4, 0}; // what we should get for corresp
    nouvtriangles(corresp, No_pts_solid, num_elem, decomp_elem, &nb_poly, coor, distance1);
    for (int i = 0; i < 9; ++i) {
        if (cor_thq1[i]!=corresp[i]) throw std::runtime_error("Failed to build the 'corresp' vector for the first case");
        corresp[i] = -1; //we reset the values of corresp for the next case
    }

    std::vector<int> cor_thq2 = {0,4,5, -1,-1,-1,-1,-1,-1};
    nouvtriangles(corresp, No_pts_solid, num_elem, decomp_elem, &nb_poly, coor, distance2);
    for (int i = 0; i < 9; ++i) {
        if (cor_thq2[i]!=corresp[i]) throw std::runtime_error("Failed to build the 'corresp' vector for the second case");
        corresp[i] =-1;
    }

    std::vector<int> cor_thq3 = {4,5,1,0, -1,-1,-1,-1,-1};
    nouvtriangles(corresp, No_pts_solid, num_elem, decomp_elem, &nb_poly, coor, distance3);
    for (int i = 0; i < 9; ++i) {
        if (cor_thq3[i]!=corresp[i]) throw std::runtime_error("Failed to build the 'corresp' vector for the third case");
    }

    double areaa = area(nb_poly, decomp_elem, distance3, coor)-1.0/2.0;
    if ((areaa>0.0001) || (areaa<-0.0001)) throw std::runtime_error("Failed to evaluate the area of the fluid domain");
}

void test_decomp()
{
    // First test //

    Vector<Point<2> >               decomp_elem(9);         // Array containing the points of the new elements created by decomposing the elements crossed by the boundary fluid/solid, there are up to 9 points that are stored in it
    int                                  nb_poly;                   // Number of sub-elements created in the fluid part for each element ( 0 if the element is entirely in the solid or the fluid)
    double                               fluid_area = 0;
    double                               area_temp;
    std::vector<Point<2> >               num_elem(6);
    std::vector<int>                     corresp(9);
    Vector<node_status>             No_pts_solid(4);

    double                               len = std::pow(2,-8); // length of the side of the square
    Point<2>                             pt1 (0,0);
    Point<2>                             pt2 (pt1(0)+len, pt1(1));
    Point<2>                             pt3 (pt1(0), pt1(1)+len);
    Point<2>                             pt4 (pt1(0)+len, pt1(1)+len);

    Vector<Point<2>>                coor;
    coor(0)=pt1;
    coor(1)=pt2;
    coor(2)=pt3;
    coor(3)=pt4;

    Vector<double>                  dist1  {0.001, 0.0005, 0.0005, -0.0005};

    nouvtriangles(corresp, No_pts_solid, num_elem, decomp_elem, &nb_poly, coor, dist1);
    Point<2> p1 (pt1(0)+len/2., pt1(1)+len);
    Point<2> p2 (pt1(0)+len, pt1(1)+len/2.);
    std::vector<Point<2>>                decomp_theo1 = {pt3, pt1, p1, pt1, p2, p1, pt2, p2, pt1};

    for (int i = 0; i < 9; ++i) {
        if (decomp_elem[i] != decomp_theo1[i])  throw std::runtime_error("Failed to create the sub-elements for the first case");
    }


    // Second test //

    double size=0;
    for (int i = 1 ; i < 4 ; ++i)
    {
      size = std::max(size,coor[0].distance(coor[i]));
    }
    double acc = size*1e-3;

    Vector<double>                  dist2  {size*1e-2, size*1e-4, size*1e-4, -size*1e-2};

    double x = (-acc + dist2[0])/(acc + dist2[0]);

    nouvtriangles(corresp, No_pts_solid, num_elem, decomp_elem, &nb_poly, coor, dist2);

    Point<2> p3 ((x+1)*len/2., 0);
    Point<2> p4 (0, (x+1)*len/2.);
    std::vector<Point<2>>                decomp_theo2 = {pt1, p3, p4};

//    for (int i = 0; i < 3; ++i) {
//        std::cout << "decomp : " << decomp_elem[0] << ", " << decomp_elem[1] << ", " << decomp_elem[2] << "\n" << std::endl;
//    }

//    for (int i = 0; i < 3; ++i) {
//      std::cout << "decomp theo : " << decomp_theo2[0] << ", " << decomp_theo2[1] << ", " << decomp_theo2[2] << "\n" << std::endl;
//    }

    for (int i = 0; i < 3; ++i) {
        if (decomp_elem[i] != decomp_theo2[i])  throw std::runtime_error("Failed to create the sub-elements for the second case");
    }

}

void test1_loop_composed_distance() // Gives the error between the calculated fluid area and the theoretical area of the fluid part
{
  MPI_Comm                         mpi_communicator(MPI_COMM_WORLD);
  unsigned int n_mpi_processes (Utilities::MPI::n_mpi_processes(mpi_communicator));
  unsigned int this_mpi_process (Utilities::MPI::this_mpi_process(mpi_communicator));

  // Create triangulation and square mesh
  parallel::distributed::Triangulation<2> triangulation (mpi_communicator, typename Triangulation<2>::MeshSmoothing
                                                         (Triangulation<2>::smoothing_on_refinement | Triangulation<2>::smoothing_on_coarsening));
  GridGenerator::hyper_cube (triangulation,
                             -2,2);

  // Refine it to get an interesting number of elements
  triangulation.refine_global(9);

  // Set-up the center, velocity and angular velocity of circle
  Point<2> center1(0.1254,0.111);
  Tensor<1,2> velocity;
  velocity[0]=1.;
  velocity[1]=0.;
  Tensor<1,3> angular;
  angular[0]=0;
  angular[1]=0;
  angular[2]=0;
  double T_scal;
  T_scal=1;
  double radius =1.0141;
  bool inside=0;

  // IB composer
  std::vector<IBLevelSetFunctions<2> *> ib_functions;
  // Add a shape to it
  IBLevelSetCircle<2> circle1(center1,velocity,angular, T_scal, inside, radius);
  ib_functions.push_back(&circle1);
  IBComposer<2> ib_composer(&triangulation,ib_functions);

  // Calculate the distance
  ib_composer.calculateDistance();

  //Get the distance into a local vector
  TrilinosWrappers::MPI::Vector levelSet_distance=ib_composer.getDistance();

  // Get DOF handler and output the IB distance field
  DoFHandler<2> *dof_handler(ib_composer.getDoFHandler());
  write_ib_scalar_data<2>(triangulation,*dof_handler,mpi_communicator,levelSet_distance,"ls_composed_distance");

  // Loop over all elements and extract the distances into a local array
  FESystem<2> *fe(ib_composer.getFESystem());
  QGauss<2>              quadrature_formula(1);
  const MappingQ<2>      mapping (1);
  std::map< types::global_dof_index, Point< 2 > > support_points;
  DoFTools::map_dofs_to_support_points ( mapping, *dof_handler,support_points );
  FEValues<2> fe_values (mapping,
                         *fe,
                         quadrature_formula,
                         update_values |
                         update_quadrature_points |
                         update_JxW_values
                         );
  const unsigned int   dofs_per_cell = fe->dofs_per_cell;         // Number of dofs per cells.
  const unsigned int   n_q_points    = quadrature_formula.size(); // quadrature on normal elements
  std::vector<types::global_dof_index> local_dof_indices (dofs_per_cell); // Global DOFs indices corresponding to cell
  Vector<Point<2> >               dofs_points(dofs_per_cell);// Array for the DOFs points
  Vector<double>  distance                  (dofs_per_cell); // Array for the distances associated with the DOFS

  Vector<Point<2> >               decomp_elem(9);         // Array containing the points of the new elements created by decomposing the elements crossed by the boundary fluid/solid, there are up to 9 points that are stored in it
  int                                  nb_poly;                   // Number of sub-elements created in the fluid part for each element ( 0 if the element is entirely in the solid or the fluid)
  double                               fluid_area = 0;
  double                               area_temp;
  double areaa = M_PI * radius * radius ;
  std::vector<Point<2> >               num_elem(6);
  std::vector<int>                     corresp(9);
  Vector<node_status>    No_pts_solid(4);

  Point<2> a;
  a[0]=0;
  a[1]=0;

  typename DoFHandler<2>::active_cell_iterator
  cell = dof_handler->begin_active(),
  endc = dof_handler->end();
  for (; cell!=endc; ++cell)
  {
    area_temp = 0.0;
    std::fill(decomp_elem.begin(), decomp_elem.end(), a);

    if (cell->is_locally_owned())
    {
      fe_values.reinit(cell);
      cell->get_dof_indices (local_dof_indices);

      for (unsigned int dof_index=0 ; dof_index < local_dof_indices.size() ; ++dof_index)
      {
        distance[dof_index] = levelSet_distance[local_dof_indices[dof_index]];
        dofs_points[dof_index] = support_points[local_dof_indices[dof_index]];
      }
    nouvtriangles(corresp, No_pts_solid, num_elem, decomp_elem, &nb_poly, dofs_points, distance);
    area_temp = area(nb_poly, decomp_elem, distance, dofs_points);
    areaa += area_temp;
        }
    }
  std::cout << " Error on the area of the fluid zone = " << areaa - 16 << std::endl;
}

int main(int argc, char* argv[])
{
  try
  {
    Utilities::MPI::MPI_InitFinalize mpi_initialization(argc, argv, numbers::invalid_unsigned_int);
    initlog();
    test0_nouv_tri();
    test1_loop_composed_distance();
    test_decomp();
  }
  catch (std::exception &exc)
  {
    std::cerr << std::endl << std::endl
              << "----------------------------------------------------"
              << std::endl;
    std::cerr << "Exception on processing: " << std::endl
              << exc.what()  << std::endl
              << "Aborting!" << std::endl
              << "----------------------------------------------------"
              << std::endl;
    return 1;
  }
  catch (...)
  {
    std::cerr << std::endl << std::endl
              << "----------------------------------------------------"
              << std::endl;
    std::cerr << "Unknown exception!" << std::endl
              << "Aborting!" << std::endl
              << "----------------------------------------------------"
              << std::endl;
    return 1;
  }

  return 0;
}
