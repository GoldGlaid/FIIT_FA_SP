#include "../include/fraction.h"

#include <cmath>
#include <numeric>
#include <regex>
#include <sstream>

big_int gcd(big_int a, big_int b) {
    if (a < 0) a = 0_bi - a;
    if (b < 0) b = 0_bi - b;

    while (b != 0) {
        big_int tmp = b;
        b = a % b;
        a = tmp;
    }

    return a;
}

void fraction::optimise() {
    if (_denominator == 0) throw std::invalid_argument("Denominator cannot be zero");

    if (_numerator == 0) {
        _denominator = 1;
        return;
    }

    big_int divisor = gcd(_numerator, _denominator);
    _numerator /= divisor;
    _denominator /= divisor;

    if (_denominator < 0) {
        _numerator = 0_bi - _numerator;
        _denominator = 0_bi - _denominator;
    }
}


fraction::fraction(const pp_allocator<big_int::value_type> allocator) : _numerator(0, allocator),
                                                                        _denominator(1, allocator) {
}


fraction &fraction::operator+=(fraction const &other) & {
    // a/b + c/d = (da + bc) / bd
    _numerator = _numerator * other._denominator + _denominator * other._numerator;
    _denominator = _denominator * other._denominator;
    optimise();
    return *this;
}

fraction fraction::operator+(fraction const &other) const {
    fraction result = *this;
    result += other;
    return result;
}

fraction &fraction::operator-=(fraction const &other) & {
    _numerator = _numerator * other._denominator - _denominator * other._numerator;
    _denominator = _denominator * other._denominator;
    optimise();
    return *this;
}

fraction fraction::operator-(fraction const &other) const {
    fraction result = *this;
    result -= other;
    return result;
}

fraction &fraction::operator*=(fraction const &other) & {
    _numerator *= other._numerator;
    _denominator *= other._denominator;
    optimise();
    return *this;
}

fraction fraction::operator*(fraction const &other) const {
    fraction result = *this;
    result *= other;
    return result;
}

fraction &fraction::operator/=(fraction const &other) & {
    if (other._numerator == 0) throw std::invalid_argument("Division by zero");

    _numerator *= other._denominator;
    _denominator *= other._numerator;
    optimise();
    return *this;
}

fraction fraction::operator/(fraction const &other) const {
    fraction result = *this;
    result /= other;
    return result;
}

fraction fraction::operator-() const {
    fraction result = *this;
    result._numerator = 0_bi - result._numerator;
    return result;
}

bool fraction::operator==(fraction const &other) const noexcept {
    return _numerator == other._numerator && _denominator == other._denominator;
}

std::partial_ordering fraction::operator<=>(const fraction &other) const noexcept {
    big_int l_val = _numerator * other._denominator;
    big_int r_val = _denominator * other._numerator;

    if (l_val < r_val) return std::partial_ordering::less;
    if (l_val > r_val) return std::partial_ordering::greater;
    return std::partial_ordering::equivalent;
}

std::string fraction::to_string() const {
    std::stringstream string;
    string << _numerator << "/" << _denominator;
    return string.str();
}

std::ostream &operator<<(std::ostream &stream, fraction const &obj) {
    return stream << obj.to_string();
}

//TODO ПЕРЕДЕЛАТЬ
std::istream &operator>>(std::istream &stream, fraction &obj) {
    std::string input;
    stream >> input;

    size_t slash_pos = input.find('/');
    bool has_slash = (slash_pos != std::string::npos);

    // Проверка на несколько слэшей
    if (input.find('/', slash_pos + 1) != std::string::npos) {
        throw std::invalid_argument("Invalid fraction format: multiple slashes");
    }

    std::string numerator_str, denominator_str;

    if (has_slash) {
        numerator_str = input.substr(0, slash_pos);
        denominator_str = input.substr(slash_pos + 1);

        // Проверка пустых частей
        if (numerator_str.empty() || denominator_str.empty()) {
            throw std::invalid_argument("Invalid fraction format: empty numerator/denominator");
        }
    } else {
        numerator_str = input;
        denominator_str = "1";
    }

    // Проверка валидности символов
    auto is_valid_number = [](const std::string& s) {
        size_t start = 0;
        if (s[start] == '+' || s[start] == '-') start++;
        return s.substr(start).find_first_not_of("0123456789") == std::string::npos;
    };

    if (!is_valid_number(numerator_str) || !is_valid_number(denominator_str)) {
        throw std::invalid_argument("Invalid fraction format: non-digit characters");
    }

    try {
        big_int numerator(numerator_str, 10);
        big_int denominator(denominator_str, 10);

        if (denominator == 0) {
            throw std::invalid_argument("Denominator cannot be zero");
        }

        obj = fraction(numerator, denominator);
    }
    catch (const std::exception& e) {
        throw std::invalid_argument(std::string("Invalid big_int conversion: ") + e.what());
    }

    return stream;
}


//term_n-1 = term
fraction fraction::sin(fraction const &epsilon) const {
    auto x = *this;
    fraction result(0, 1);
    fraction term = x;
    int n = 1;
    fraction prev_result;

    do {
        prev_result = result;
        result += term;
        term = term * (-x * x) / fraction((2 * n) * (2 * n + 1), 1);
        n++;
    } while ((result - prev_result > epsilon) || (prev_result - result > epsilon));

    return result;
}


fraction fraction::cos(fraction const &epsilon) const {
    auto x = *this;
    fraction result(1, 1);
    fraction term(1, 1);
    int n = 1;

    fraction tmp_result;
    do {
        tmp_result = result;
        term = term * (-x * x) / fraction((2 * n - 1) * (2 * n), 1);
        result += term;
        n++;
    } while (result - tmp_result > epsilon || tmp_result - result > epsilon);

    return result;
}

fraction fraction::tg(fraction const &epsilon) const {
    fraction sine = sin(epsilon * epsilon);
    fraction cosine = cos(epsilon * epsilon);

    if (cosine._numerator == 0) {
        throw std::domain_error("Tangent undefined");
    }

    return sine / cosine;
}

fraction fraction::ctg(fraction const &epsilon) const {
    fraction cosine = cos(epsilon * epsilon);
    fraction sine = sin(epsilon * epsilon);

    if (sine._numerator == 0) {
        throw std::domain_error("Cotangent undefined");
    }

    return cosine / sine;
}

fraction fraction::arcsin(const fraction &epsilon) const {
    if (*this < fraction(-1, 1) || *this > fraction(1, 1)) {
        throw std::domain_error("Arcsin undefined for |x| > 1");
    }

    fraction x = *this;
    fraction result(0, 1);
    fraction term = x;
    big_int n = 1;
    fraction tmp_result;

    do {
        tmp_result = result;
        result += term;
        term = term * x * x * fraction((2_bi * n - 1) * (2_bi * n - 1), (2_bi * n) * (2_bi * n + 1));
        n += 1;
    } while ((result - tmp_result > epsilon) || (tmp_result - result > epsilon));

    return result;
}

fraction fraction::arccos(fraction const &epsilon) const {
    return fraction(1, 2).arcsin(epsilon) * fraction(3, 1) - this->arcsin(epsilon);
}

fraction fraction::arctg(fraction const &epsilon) const {
    auto x = *this;

    if (*this > fraction(1, 1)) {
        return fraction(1, 2).arcsin(epsilon) * fraction(3, 1) -
               (fraction(1, 1) / *this).arctg(epsilon);
    }

    fraction result(0, 1);
    fraction term = *this;
    big_int n = 1;
    fraction tmp_result;

    do {
        tmp_result = result;
        result += fraction(1, n) * term;
        n += 2_bi;
        term = -term * (x * x);
    } while ((result - tmp_result > epsilon) || (tmp_result - result > epsilon));

    return result;
}

fraction fraction::arcctg(fraction const &epsilon) const {
    if (_numerator == 0) {
        throw std::domain_error("Arccotangent undefined");
    }

    return (fraction(1, 1) / *this).arctg(epsilon);
}

fraction fraction::sec(fraction const &epsilon) const {
    fraction cosine = cos(epsilon);

    if (cosine._numerator == 0) {
        throw std::domain_error("Secant undefined");
    }

    return fraction(1, 1) / cosine;
}

fraction fraction::cosec(fraction const &epsilon) const {
    fraction sine = sin(epsilon);

    if (sine._numerator == 0) {
        throw std::domain_error("Cosecant undefined");
    }

    return fraction(1, 1) / sine;
}


fraction fraction::arcsec(fraction const &epsilon) const {
    if (_numerator == 0) {
        throw std::domain_error("Arcsecant undefined");
    }

    fraction reciprocal = fraction(1, 1) / *this;
    return reciprocal.arccos(epsilon);
}

fraction fraction::arccosec(fraction const &epsilon) const {
    if (_numerator == 0) {
        throw std::domain_error("Arccosecant undefined");
    }

    fraction reciprocal = fraction(1, 1) / *this;
    return reciprocal.arcsin(epsilon);
}


fraction fraction::pow(size_t degree) const {
    if (degree < 0) throw std::invalid_argument("Degree must be positive");
    if (degree == 0) return {1, 1};

    fraction base = *this;
    fraction result(1, 1);
    while (degree > 0) {
        if (degree & 1) result *= base;

        base *= base;
        degree >>= 1;
    }

    return result;
}

fraction fraction::root(size_t n, fraction const &epsilon) const {
    if (n <= 0) throw std::invalid_argument("Degree must be more than 0");
    if (n == 1) return *this;
    if (_numerator < 0 && n % 2 == 0)
        throw std::domain_error("Even root of negative number is not real");
    //this = A
    fraction x = *this;
    if (x._numerator < 0) x = -x;

    fraction result = *this / fraction(n, 1);
    fraction prev_result;
    do {
        prev_result = result;
        fraction power = result.pow(n - 1); // x ^ n-1
        if (power._numerator == 0) throw std::logic_error("Division by zero in root calculation");

        result = (fraction(n - 1, 1) * result + *this / power) / fraction(n, 1);
    } while ((result - prev_result > epsilon) || (prev_result - result > epsilon));

    if (_numerator < 0 && n % 2 == 1) result = -result;

    return result;
}

fraction fraction::log2(fraction const &epsilon) const {
    if (_numerator <= 0 || _denominator <= 0) throw std::domain_error("Logarithm of non-positive number is undefined");

    return ln(epsilon) / fraction(2, 1).ln(epsilon);
}

fraction fraction::ln(fraction const &epsilon) const {
    if (_numerator <= 0 || _denominator <= 0) {
        throw std::domain_error("Natural logarithm of non-positive number is undefined");
    }

    fraction y = (*this - fraction(1, 1)) / (*this + fraction(1, 1));
    fraction y_squared = y * y;
    fraction term = y;
    fraction result = term;
    int n = 1;

    fraction tmp_result;
    do {
        tmp_result = result;
        term = term * y_squared;
        n += 2;
        result += term / fraction(n, 1);
    } while (result - tmp_result > epsilon || tmp_result - result > epsilon);

    return result * fraction(2, 1);
}

fraction fraction::lg(fraction const &epsilon) const {
    if (_numerator <= 0 || _denominator <= 0)
        throw std::domain_error(
            "Base-10 logarithm of non-positive number is undefined");

    return this->ln(epsilon) / fraction(10, 1).ln(epsilon);
}
