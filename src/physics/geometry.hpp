#pragma once

#include <vector>


struct TPoint {
	float x, y;
	bool isWallFollows = true; // ��� ������� ������������� �������

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
	bool isWall;

	TFace(TPoint _beg, TPoint _end, bool _isWall) : beg(_beg), end(_end), isWall(_isWall) { //    normal
		tangent = _end - _beg;                                                              //    ^
		tangent.normalize();                                                                //    |
		normal = { tangent.y, - tangent.x }; // ����� �������                                     O----> tangent    ��! ��� y ���������� ����!
	}

	TPoint intersect(TFace other) const {
		float num = (other.beg - beg).crossComponent(other.tangent);
		float den = tangent.crossComponent(other.tangent);  // != 0
		float t = num / den;
		return beg + tangent * t;
	}

	TFace shiftOut(const float distance) const {
		return TFace(beg + normal * distance, end + normal * distance, isWall);
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

	bool isInside(const TPoint& pnt) const {
		return isPointOnTheRight(pnt) || isPointWithinTheScope(pnt);
	}

	// ������������ ������� ����������, ��� ������� ����� ����� �� ����� pnt
	float observationAngle(const TPoint& pnt) const {
		TPoint v1 = beg - pnt;
		TPoint v2 = end - pnt;
		float sineSign = sign(v1.crossComponent(v2));
		float cosine = (v1 * v2) / (v1.norm() * v2.norm());
		// ���� ���� ������� ���, cosine ����� ���� =1.00000045...
		// � ����� �� float ����� �������� � ����, � acos ����� nan(ind) --> ��������� �����
		cosine = std::round(cosine*1e6) / 1e6;
		return sineSign * acos(cosine);
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
			faces.push_back({ beg, end, beg.isWallFollows });
		}
	}

	int get_next_idx(int i) const {
		return (i == coords.size() - 1) ? 0 : i + 1;
	}

	int get_prev_idx(int i) const {
		return (i == 0) ? coords.size() - 1 : i - 1;
	}

	// ����� ���� �����, ��� �������� ����� ����� ��������� �� ����� pnt, 
	// ����� 360 ��������, ���� ����� ������, � 0 ��������, ���� ����� �������
	bool isInside(const TPoint& pnt) const {
		float angle = 0.0f;
		for (auto face : faces)
			angle += face.observationAngle(pnt);
		// ������������� �� ������ �������� 0.0, ���� ����� ��������� �������, 
		// � 2*PI, ���� ������.
		// ������, ���� ����� ������ � ��������� ����������� ����������� ����� ���������,
		// ����������� ������� ���� �� float ���������� --> 0 << 0.1 << 2*PI
		return (abs(angle) < 0.1f) ? false : true;
	}

	float _DEBUG_isInsideRetAngle(const TPoint& pnt) const {
		float angle = 0.0f;
		for (auto face : faces)
			angle += face.observationAngle(pnt);

		return angle;
	}

	// Inflate geometry moving all edges outwards by a given distance
	std::vector<TPoint> getCoordsInflated(const float distance = 0.5) {
		std::vector<TFace> edges;
		std::vector<TPoint> coordsInflated;

		// ������� �����
		for (int i{ 0 }; i < coords.size(); ++i) {
			const TPoint& beg = coords[i];
			const TPoint& end = coords[get_next_idx(i)];
			TFace edge = { beg, end, beg.isWallFollows };
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