#pragma once

#include <vector>


struct TPoint {
	float x, y;
	bool isWallFollows = true; // для удобной инициализации границы

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

	// z-составляющая векторного произведения двух векторов из плоскости (x,y)
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
	// point_on_line = p1 + (p2 - p1) * t, t -- параметр
	TPoint beg, end;
	TPoint tangent, normal;
	bool isWall;

	TFace(TPoint _beg, TPoint _end, bool _isWall) : beg(_beg), end(_end), isWall(_isWall) { //    normal
		tangent = _end - _beg;                                                              //    ^
		tangent.normalize();                                                                //    |
		normal = { tangent.y, - tangent.x }; // левая нормаль                                     O----> tangent    НО! ось y направлена вниз!
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

	// Определяем, лежит ли точка pnt справа от нас (от данной грани)
	// Для этого используем свойство векторного произведения [tangent, beg_pnt]
	bool isPointOnTheRight(const TPoint& pnt) const {
		const TPoint vec = pnt - beg;
		return (tangent.x * vec.y - tangent.y * vec.x) > 0; // в графике ось x направлена вправо, но ось y направлена вниз!
	}

	bool isPointWithinTheScope(const TPoint& pnt) const {
		const TPoint bp = pnt - beg;
		const TPoint be = pnt - end;
		return (tangent * bp < 0.0) || (tangent * be > 0.0); // область, которую заметает грань, двигаясь в направлении своей нормали 
	}

	bool isInside(const TPoint& pnt) const {
		return isPointOnTheRight(pnt) || isPointWithinTheScope(pnt);
	}

	// Рассчитывает угловое расстояние, под которым видна грань из точки pnt
	float observationAngle(const TPoint& pnt) const {
		TPoint v1 = beg - pnt;
		TPoint v2 = end - pnt;
		float sineSign = sign(v1.crossComponent(v2));
		float cosine = (v1 * v2) / (v1.norm() * v2.norm());
		// Если угол слишком мал, cosine может быть =1.00000045...
		// а мусор во float может привести к тому, и acos вернёт nan(ind) --> отбросить мусор
		cosine = std::round(cosine*1e6) / 1e6;
		return sineSign * acos(cosine);
	}
};


struct TGeometry {
	std::vector<TPoint> coords;
	std::vector<TFace> faces;

	TGeometry(std::vector<TPoint> _coords) : coords(_coords) {
		// Формируем грани
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

	// Сумма всех углов, под которыми видно грани геометрии из точки pnt, 
	// равна 360 градусов, если точка внутри, и 0 градусов, если точка снаружи
	bool isInside(const TPoint& pnt) const {
		float angle = 0.0f;
		for (auto face : faces)
			angle += face.observationAngle(pnt);
		// Математически мы должны получить 0.0, если точка находится снаружи, 
		// и 2*PI, если внутри.
		// Однако, если точка попала в небольшую окрестность продолжения линий геометрии,
		// погрешность расчёта угла во float возрастает --> 0 << 0.1 << 2*PI
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

		// Готовим грани
		for (int i{ 0 }; i < coords.size(); ++i) {
			const TPoint& beg = coords[i];
			const TPoint& end = coords[get_next_idx(i)];
			TFace edge = { beg, end, beg.isWallFollows };
			TFace edgeShifted = edge.shiftOut(distance);
			edges.push_back(edgeShifted);
		}

		// Готовим новую геометрию, пересекая сдвинутые грани
		for (int i{ 0 }; i < edges.size(); ++i) {
			const TFace first = edges[get_prev_idx(i)];
			const TFace second = edges[i];
			coordsInflated.push_back(first.intersect(second));
		}

		return coordsInflated;
	}
};