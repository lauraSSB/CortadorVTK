// =========================================================================
// Base example to interactively cut a volume with a plane
// =========================================================================

#include <fstream>
#include <iostream>
#include <string>

#include <vtkColorTransferFunction.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkMetaImageReader.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

int main( int argc, char** argv )
{
  if( argc < 3 )
  {
    std::cout
      << "Usage: " << argv[ 0 ]
      << " file.[mhd|mha] volume_properties.txt"
      << std::endl;
    return( EXIT_FAILURE );
  } // end if
  std::string input_filename = argv[ 1 ];
  std::string vp_filename = argv[ 2 ];

  // Read properties file
  std::ifstream vp_file( vp_filename, std::ios::binary | std::ios::ate );
  if( !vp_file )
  {
    std::cerr << "Could not open file: " << vp_filename << std::endl;
    return( EXIT_FAILURE );
  } // end if
  std::streamsize vp_file_size = vp_file.tellg( );
  vp_file.seekg( 0, std::ios::beg );
  std::string vp( static_cast< std::size_t >( vp_file_size ), '\0' );
  if( !vp_file.read( vp.data( ), vp_file_size ) )
  {
    std::cerr << "Could not read file: " << vp_filename << std::endl;
    return( EXIT_FAILURE );
  } // end if

  // Create volume raycasting properties
  vtkNew< vtkColorTransferFunction > volume_color;
  vtkNew< vtkPiecewiseFunction > volume_opacity;
  vtkNew< vtkPiecewiseFunction > volume_gradient;
  vtkNew< vtkVolumeProperty > volume_properties;
  std::istringstream vp_str( vp );
  std::string line;
  auto trim = []( const std::string& s ) -> std::string
    {
      const char* ws = " \t";
      std::size_t start = s.find_first_not_of( ws );
      if( start == std::string::npos )
        return( "" );
      std::size_t end = s.find_last_not_of( ws );
      return( s.substr( start, end - start + 1 ) );
    };
  while( std::getline( vp_str, line ) )
  {
    std::string l = trim( line );
    if( l != "" )
    {
      std::size_t p = l.find_first_of( " \t" );
      if( p != std::string::npos )
      {
        std::string cmd = l.substr( 0, p );
        std::istringstream values( l.substr( p ) );
        if( cmd == "color" )
        {
          double i, r, g, b;
          values >> i >> r >> g >> b;
          volume_color->AddRGBPoint( i, r, g, b );
        }
        else if( cmd == "opacity" )
        {
          double i, v;
          values >> i >> v;
          volume_opacity->AddPoint( i, v );
        }
        else if( cmd == "gradient" )
        {
          double i, v;
          values >> i >> v;
          volume_gradient->AddPoint( i, v );
        }
        else if( cmd == "shade" )
        {
          std::string v;
          values >> v;
          if( v == "on" )
            volume_properties->ShadeOn( );
          else if( v == "off" )
            volume_properties->ShadeOff( );
        }
        else if( cmd == "ambient" )
        {
          double v;
          values >> v;
          volume_properties->SetAmbient( v );
        }
        else if( cmd == "diffuse" )
        {
          double v;
          values >> v;
          volume_properties->SetDiffuse( v );
        }
        else if( cmd == "specular" )
        {
          double v;
          values >> v;
          volume_properties->SetSpecular( v );
        } // end if
      } // end if
    } // end if
  } // end while

  // Read a 3D image (volume)
  vtkNew< vtkMetaImageReader > volume_reader;
  volume_reader->SetFileName( argv[ 1 ] );

  // Prepare ray-caster
  vtkNew< vtkFixedPointVolumeRayCastMapper > volume_mapper;
  volume_mapper->SetInputConnection( volume_reader->GetOutputPort( ) );

  // Volume rendering properties
  volume_properties->SetColor( volume_color );
  volume_properties->SetScalarOpacity( volume_opacity );
  volume_properties->SetGradientOpacity( volume_gradient );
  volume_properties->SetInterpolationTypeToLinear( );

  // Raycast actor
  vtkNew< vtkVolume > volume;
  volume->SetMapper( volume_mapper );
  volume->SetProperty( volume_properties );

  // Prepare rendering objects
  vtkNew< vtkInteractorStyleSwitch > style;
  vtkNew< vtkRenderer > ren;
  vtkNew< vtkRenderWindow > win;
  vtkNew< vtkRenderWindowInteractor > iren;

  style->SetCurrentStyleToTrackballCamera( );
  ren->SetBackground( 0.7, 0.7, 0.7 );
  win->AddRenderer( ren );
  iren->SetRenderWindow( win );
  iren->SetInteractorStyle( style );

  // Add actors
  ren->AddViewProp( volume );

  // Finish setup and start main loop
  win->SetSize( 800, 800 );
  win->SetWindowName( "Volume slicer" );
  win->Render( );
  iren->Start( );

  return( EXIT_SUCCESS );
}

// eof - VolumeSlicer.cxx
