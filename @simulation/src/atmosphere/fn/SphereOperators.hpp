#pragma once

#include "atmosphere/type/SphereMatrix.hpp"
#include "atmosphere/type/SphereStencil.hpp"
#include "geometry/type/SphereGrid.hpp"

#include <vector>

namespace derivean::sim {

// Finite-volume operators on the weather grid. Scalars live at cell centres (cell means); vectors
// and fluxes live on edges, one value per shared edge, oriented out of the edge's `left` cell and
// applied to `right` with the opposite sign. The stencil must be the grid's own (makeSphereStencil of
// that grid; asserted by size). Every operator is the same formula on every edge, so
// face seams and cube vertices differ from the face interior only through their metric.
using CellField = std::vector<double>;     // per cell (per vertex from vertexValues)
using EdgeField = std::vector<double>;       // per edge
using EdgeVectors = std::vector<Vec3>;       // per edge, tangent ECEF vectors at the edge midpoint

// Per vertex: the linear-exact weighted value of the cells meeting there (type/SphereStencil.hpp).
CellField vertexValues(const SphereGrid& grid, const SphereStencil& stencil, const CellField& field);
void vertexValues(const SphereGrid& grid, const SphereStencil& stencil, const CellField& field, CellField& result);

// The gradient at every edge midpoint (1/m times the field's unit): the difference across the edge
// plus the difference along it from vertex values, which is the gnomonic grid's non-orthogonal term.
EdgeVectors gradientOf(const SphereGrid& grid, const SphereStencil& stencil, const CellField& field);

// ∫ w · n dl through every edge, out of left (m × the vector's unit).
EdgeField normalFluxOf(const SphereGrid& grid, const EdgeVectors& vectors);

// Per cell: the sum of its edges' outward fluxes over its area. Σ area · divergence over the closed
// grid cancels edge by edge.
CellField divergenceOf(const SphereGrid& grid, const EdgeField& flux);

// Per cell: the circulation of edge vectors round the cell over its area, each edge contributing
// w · chord (counter-clockwise for left, clockwise for right). curl(gradient) is zero to rounding.
CellField curlOf(const SphereGrid& grid, const SphereStencil& stencil, const EdgeVectors& vectors);

// The volume flux (m²/s per unit depth) through every edge of the non-divergent wind v = up × ∇ψ of
// a stream function ψ (m²/s): ψ(from) − ψ(to) at the edge's vertices. Round a cell these telescope,
// so the wind's divergence is zero to rounding in every cell, and a uniform tracer stays uniform.
EdgeField streamFluxOf(const SphereGrid& grid, const SphereStencil& stencil, const CellField& stream);

// The same into `flux`, with ψ at the vertices formed once into `atVertex` (work space): each vertex
// serves four edges, and the fluxes are the same bit for bit.
void streamFluxOf(const SphereGrid& grid, const SphereStencil& stencil, const CellField& stream, EdgeField& flux, CellField& atVertex);

// The Laplacian's stiffness K = Σ_edges diamond · Gᵀ G, with G the edge gradient above: symmetric by
// construction, positive semi-definite, and zero on constants to rounding (each G row sums to zero). It
// is the energy ½ Σ diamond |∇ψ|² of the discrete duality finite-volume method (Hermeline 2000;
// Domelevo & Omnes 2005) with the vertex unknowns replaced by their least-squares cell values
// (SphereStencil::weight), so it is a cell-centred operator CG can invert.
SphereMatrix makeLaplacian(const SphereGrid& grid, const SphereStencil& stencil);

// Δψ per cell = −(K ψ) / area.
void applyMatrix(const SphereMatrix& matrix, const CellField& field, CellField& result);
CellField laplacianOf(const SphereGrid& grid, const SphereMatrix& stiffness, const CellField& field);

// Σ a · b over the cells (no area weight): the inner product the solvers use.
double dotOf(const CellField& a, const CellField& b);

// Area-weighted mean over the sphere.
double areaMean(const SphereGrid& grid, const CellField& field);

} // namespace derivean::sim
