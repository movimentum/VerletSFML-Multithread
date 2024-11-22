#pragma once

#include <vector>


struct TPoint {
	float x, y;
	bool isWallFollows = true;

	TPoint operator-(const TPoint& other) const {
		return { x - other.x, y - other.y };
	}

	float operator*(const TPoint& other) const {
		return x * other.x + y * other.y;
	}

	TPoint operator*(const float factor) const {
		return { x * factor, y * factor };
	}

	TPoint operator+(const TPoint& other) const {
		return { x + other.x, y + other.y };
	}

	// z-������������ ���������� ������������ ���� �������� �� ��������� (x,y)
	float crossComponent(const TPoint& other) const {
		return y * other.x - x * other.y;
	}

	float norm() const {
		return sqrt(x * x + y * y);
	}

	void normalize() {
		float n = norm();
		x /= n;
		y /= n;
	}
};

struct TFace {
	// point_on_line = p1 + (p2 - p1) * t, t -- ��������
	TPoint beg, end;
	TPoint tangent, normal;

	TFace(TPoint _beg, TPoint _end) : beg(_beg), end(_end) {      //    normal
		tangent = _end - _beg;                                    //    ^
		tangent.normalize();                                      //    |
		normal = { tangent.y, - tangent.x }; // ����� �������           O----> tangent    ��! ��� y ���������� ����!
	}

	TPoint intersect(TFace other) const {
		float num = (other.beg - beg).crossComponent(other.tangent);
		float den = tangent.crossComponent(other.tangent);  // != 0
		float t = num / den;
		return beg + tangent * t;
	}

	TFace shiftOut(const float distance) const {
		return TFace(beg + normal * distance, end + normal * distance);
	}

	// ����������, ����� �� ����� pnt ������ �� ��� (�� ������ �����)
	// ��� ����� ���������� �������� ���������� ������������ [tangent, beg_pnt]
	bool isPointOnTheRight(const TPoint& pnt) const {
		const TPoint vec = pnt - beg;
		return (tangent.x * vec.y - tangent.y * vec.x) > 0; // � ������� ��� x ���������� ������, �� ��� y ���������� ����!
	}

	bool isPointWithinTheScope(const TPoint& pnt) const {
		const TPoint bp = pnt - beg;
		const TPoint be = pnt - end;
		return (tangent * bp < 0.0) || (tangent * be > 0.0); // �������, ������� �������� �����, �������� � ����������� ����� ������� 
	}
};


struct TGeometry {
	std::vector<TPoint> coords;
	std::vector<TFace> faces;

	TGeometry(std::vector<TPoint> _coords) : coords(_coords) {
		// ��������� �����
		for (int i{ 0 }; i < coords.size(); ++i) {
			const TPoint& beg = coords[i];
			const TPoint& end = coords[get_next_idx(i)];
			faces.push_back({ beg, end });
		}
	}

	int get_next_idx(int i) const {
		return (i == coords.size() - 1) ? 0 : i + 1;
	}

	int get_prev_idx(int i) const {
		return (i == 0) ? coords.size() - 1 : i - 1;
	}

	bool isInside(const TPoint& pnt, const int iFace) const {
		const TFace& face = faces[iFace];
		return face.isPointOnTheRight(pnt) || face.isPointWithinTheScope(pnt);
	}

	bool isInside(const TPoint& pnt) const {
		//for (int i{ 0 }; i < coords.size(); ++i) {
		for (auto face : faces){
			if ( ! (face.isPointOnTheRight(pnt)) )
				return false;
		}
		return true;
	}

	// ���������� �� � ������ ����� ������
	bool isWall(int wallBegIdx) {
		return coords[wallBegIdx].isWallFollows;
	}

	// Get face
	TPoint getFace(const int iBeg) const {
		const TPoint& beg = coords[iBeg];
		const TPoint& end = coords[get_next_idx(iBeg)];
		return end - beg;
	}

	// Inflate geometry moving all edges outwards by a given distance
	std::vector<TPoint> getCoordsInflated(const float distance = 0.5) {
		std::vector<TFace> edges;
		std::vector<TPoint> coordsInflated;

		// ������� �����
		for (int i{ 0 }; i < coords.size(); ++i) {
			const TPoint& beg = coords[i];
			const TPoint& end = coords[get_next_idx(i)];
			TFace edge = { beg, end };
			TFace edgeShifted = edge.shiftOut(distance);
			edges.push_back(edgeShifted);
		}

		// ������� ����� ���������, ��������� ��������� �����
		for (int i{ 0 }; i < edges.size(); ++i) {
			const TFace first = edges[get_prev_idx(i)];
			const TFace second = edges[i];
			coordsInflated.push_back(first.intersect(second));
		}

		return coordsInflated;
	}
};