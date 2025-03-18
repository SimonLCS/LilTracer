#include "micrograin.h"


namespace LT_NAMESPACE {

    Float MicrograinMicrosurface::one_to_many(const Float& tau_0, const Float& sigma_) {
        Float rho = -std::log(1 - tau_0) / pi;
        return  std::exp(-rho * sigma_);
    }

    Float MicrograinMicrosurface::tau_v(const Float& tau_0, const vec3& wi_u) {
        return 1 - one_to_many(tau_0, sigma(wi_u));
    }

    Float MicrograinMicrosurface::D(const vec3& wh_u, const SurfaceInteraction& si) {
        Float tau = tau_0->eval(si);
        Float rho = -std::log(1 - tau) / pi;
        float D_ = rho * one_to_many(tau, sigma_base(wh_u)) / (tau);
        return D_;
    }

    Float MicrograinMicrosurface::D(const vec3& wh_u, const vec3& wi_u, const SurfaceInteraction& si) {
        return 0.;
    }

    Float MicrograinMicrosurface::pdf(const vec3& wh_u, const SurfaceInteraction& si)
    {
        return D(wh_u, si) * wh_u.z;
    }
    Float MicrograinMicrosurface::pdf(const vec3& wh_u, const vec3& wi_u, const SurfaceInteraction& si) {
        return D(wh_u, wi_u, si);
    }

    vec3 MicrograinMicrosurface::sample_D(Sampler& sampler, const SurfaceInteraction& si)
    {
        Float tau = tau_0->eval(si);
        float u = sampler.next_float();
        // warp the uniform distribution based on the filling factor
        float v = std::log(1. - u * tau) / std::log(1. - tau);

        return square_to_cosine_hemisphere(v, sampler.next_float());
    }



    vec3 MicrograinMicrosurface::sample_D(const vec3& wi_u, Sampler& sampler, const SurfaceInteraction& si)
    {   
        return vec3(0.,0.,0.);
    }

    Float MicrograinMicrosurface::lambda(const Float& tau_0, const vec3& wi_u)
    {
        constexpr Float beta = 1.;
        constexpr Float beta2 = 1.;
        constexpr Float beta4 = 1.;
        constexpr Float beta6 = 1.;
        constexpr Float beta8 = 1.;
        constexpr Float beta10 = 1.;
        constexpr Float beta12 = 1.;
        
        Float pi_gamma = -std::log(1. - tau_0);
        Float exp_pi_gamma_minus_one = std::exp(pi_gamma) - 1.;

        Float cos_theta = glm::clamp(wi_u.z, 0.00001f, 0.99999f);
        Float mu = cos_theta / std::sqrt(1. - cos_theta * cos_theta);

        Float mu2 = mu * mu;
        Float mu4 = mu2 * mu2;
        Float mu6 = mu4 * mu2;
        Float mu8 = mu6 * mu2;
        Float mu10 = mu8 * mu2;
        Float mu12 = mu10 * mu2;

        Float beta2_mu2 = beta2 + mu2;
        Float sqrt_beta2_mu2 = std::sqrt(beta2_mu2);

        Float F0 = std::pow(pi_gamma, 1) * (-mu + sqrt_beta2_mu2) / (2 * mu);
        
        Float F1 = std::pow(pi_gamma, 2) * (beta2 + 2 * mu * (mu - sqrt_beta2_mu2)) / (8 * mu * sqrt_beta2_mu2);

        Float F2 = std::pow(pi_gamma, 3) * (3 * beta4 + 12 * beta2 * mu2 + 8 * mu4 - 8 * mu * std::pow(beta2_mu2,3. / 2.)) / (96 * mu * std::pow(beta2_mu2, 3. / 2.));

        Float F3 = std::pow(pi_gamma, 4) * (5 * beta6 + 30 * beta4 * mu2 + 40 * beta2 * mu4 + 16 * mu6 - 16 * mu * std::pow(beta2_mu2, 5./ 2.)) / (768 * mu * std::pow(beta2_mu2, 5. / 2.));

        Float F4 = std::pow(pi_gamma, 5) * (35 * beta8 + 280 * beta6 * mu2 + 560 * beta4 * mu4 + 448 * beta2 * mu6 + 128 * mu8 - 128 * mu * std::pow(beta2_mu2, 7. / 2.)) / (30720 * mu * std::pow(beta2_mu2, 7. / 2.));
            
        Float F5 = std::pow(pi_gamma, 6) * (63 * beta10 + 630 * beta8 * mu2 + 1680 * beta6 * mu4 + 2016 * beta4 * mu6 + 1152 * beta2 * mu8 + 256 * mu10 - 256 * mu * std::pow(beta2_mu2, 9. / 2.)) / (368640 * mu * std::pow(beta2_mu2, 9. / 2.));

        Float F6 = std::pow(pi_gamma, 7) * (231 * beta12 + 2772 * beta10 * mu2 + 9240 * beta8 * mu4 + 14784 * beta6 * mu6 + 12672 * beta4 * mu8 + 5632 * beta2 * mu10 + 1024 * mu12 - 1024 * mu * std::pow(beta2_mu2, 11. / 2.)) / (10321920 * mu * std::pow(beta2_mu2, 11. / 2.));

        return (F0 + F1 + F2 + F3 + F4 + F5 + F6) / exp_pi_gamma_minus_one;
    }

    Float MicrograinMicrosurface::sigma(const vec3& wi_u) 
    {
        return pi * 0.5 * (1. + 1. / glm::clamp(wi_u.z,0.00001f,0.99999f));
    }
    
    Float MicrograinMicrosurface::w_plus(const Float& tau_0, const vec3& wi_u, const vec3& wo_u) {
        Float si_plus_so_minus_sn = pi * 0.5 * (1 / glm::clamp(wi_u.z, 0.00001f, 0.99999f) + 1 / glm::clamp(wo_u.z, 0.00001f, 0.99999f));
        return 1 - std::exp(std::log(1. - tau_0) / pi  * si_plus_so_minus_sn);
    }

    Float MicrograinMicrosurface::sigma_base(const vec3& wh_u) 
    {
        return pi * (1 - wh_u[2] * wh_u[2]);
    }

    Float MicrograinMicrosurface::sigma_shadow(const vec3& wh_u, const vec3& wi_u)
    {
        if (wi_u.z >= 0.99999)
            return 0.;

        Float cos_theta_i_sqr = wi_u.z * wi_u.z;
        Float sin_theta_i_sqr = 1. - cos_theta_i_sqr;
        if (wh_u.z >= std::sqrt(sin_theta_i_sqr))
            return 0.;

        Float tan_theta_i_sqr = sin_theta_i_sqr / cos_theta_i_sqr;

        Float cos_theta_m_sqr = wh_u.z * wh_u.z;
        Float sin_theta_m_sqr = 1. - cos_theta_m_sqr;
        Float tan_theta_m_sqr = sin_theta_m_sqr / cos_theta_m_sqr;

        Float cos_phi_q = -1. / std::sqrt(tan_theta_i_sqr * tan_theta_m_sqr);

        Float q_x = std::sqrt(sin_theta_m_sqr) * cos_phi_q;
        Float q_y = std::sqrt(sin_theta_m_sqr * (1. - cos_phi_q * cos_phi_q));
        Float e_x = std::sqrt(tan_theta_i_sqr) * wh_u.z;
        Float sigma_e = std::acos(glm::clamp(-(q_x - e_x) * wi_u.z,0.f,1.f)) / wi_u.z;
        Float sigma_t = std::abs(e_x * q_y);
        Float sigma_c = (pi - std::acos(cos_phi_q)) * sin_theta_m_sqr;

        return std::max(sigma_e - sigma_t - sigma_c,0.f);
    }

    inline glm::vec2 shadow_intersection_point(const vec3& wh_u, const vec3& wi_u, const vec3& wo_u, const vec3& t, const vec3& b) {
        Float a0 = -t[2] / b[2];
        Float a1 = wh_u[2] / b[2];
        Float a0_sqr = a0 * a0;

        Float d = glm::dot(b, wo_u);
        Float sy = 1. / glm::sqrt(1. - glm::clamp(d*d, 0.00001f, 0.99999f));
        Float sy_sqr = sy * sy;

        Float x = (-a0 * a1 / sy_sqr - glm::sqrt((a0_sqr - a1 * a1) / sy_sqr + 1.)) / (a0_sqr / sy_sqr + 1.);
        Float y = a0 * x + a1;

        vec3 p = t * x + b * y;
        return glm::vec2(p[0], p[1]);
    }

    inline glm::vec2 shadow_intersection_point_0(const vec3& wi, const vec3& wo, const vec3& t, const vec3& b) {
        Float a0 = -t[2] / b[2];
        Float a0_sqr = a0 * a0;

        Float d = glm::dot(b, wo);
        Float sy = 1. / glm::sqrt(1. - glm::clamp(d * d, 0.00001f, 0.99999f));
        Float sy_sqr = sy * sy;

        Float x = (- glm::sqrt(a0_sqr / sy_sqr + 1.)) / (a0_sqr / sy_sqr + 1.);
        Float y = a0 * x;

        vec3 p = t * x + b * y;
        return glm::vec2(p[0], p[1]);
    }

    inline Float area_triangle(const vec2& a, const vec2& b, const vec2& c) {
        Float area = a[0] * (b[1] - c[1]) + b[0] * (c[1] - a[1]) + c[0] * (a[1] - b[1]);
        return 0.5 * std::abs(area);
    }


    inline Float area_sector(const vec2& p1, const vec2& p2, const Float& rad) {
        Float cos_theta = glm::clamp(glm::dot(p1, p2) / (glm::length(p1) * glm::length(p2)), -0.99999f, 0.99999f);
        return (std::acos(cos_theta) * rad * rad - std::abs(p1[0] * p2[1] - p2[0] * p1[1])) * 0.5;
    }

    inline Float area_sector_i(const vec2& p, const vec2& pi, const Float& cos_theta_u, const vec3& wi_u) {
        vec2 direction = glm::normalize(vec2(wi_u[0], wi_u[1]));

        Float cos_theta_i = glm::max(wi_u[2], 0.0001f);
        Float tan_theta_i = glm::sqrt(1 - cos_theta_i * cos_theta_i) / cos_theta_i;
        vec2 centered_p = p - direction * tan_theta_i * cos_theta_u;
        vec2 centered_pi = pi - direction * tan_theta_i * cos_theta_u;

        glm::mat2 E = glm::mat2(direction / cos_theta_i, vec2(-direction[1], direction[0]));
        glm::mat2 inv_E = glm::inverse(E);

        centered_p = inv_E * centered_p;
        centered_pi = inv_E * centered_pi;

        return area_sector(centered_p, centered_pi, 1) * glm::determinant(E);
    }

    inline void silhouette_points(const vec3& wh_u, const vec3 wi_u, const vec3& wo_u, vec2& pi_plus, vec2& pi_minus) {
        Float cos_th_i_sqr = wi_u[2]* wi_u[2];
        Float sin_th_i_sqr = 1. - cos_th_i_sqr;
        Float cos_th_m_sqr = wh_u[2] * wh_u[2];
        Float tan_th_i = std::sqrt(sin_th_i_sqr/ cos_th_i_sqr);
        Float sign = glm::sign(wi_u[0] * wo_u[1] - wo_u[0] * wi_u[1]);

        Float x_qi = std::sqrt(1 - cos_th_m_sqr / sin_th_i_sqr) * sign;
        Float y_qi = -wh_u[2] / tan_th_i;
        
        vec2 wi_u_proj = glm::normalize(vec2(wi_u.x, wi_u.y));
        Float sin_ph_i = wi_u_proj.y;
        Float cos_ph_i = wi_u_proj.x;
        pi_plus  = vec2( x_qi * sin_ph_i + y_qi * cos_ph_i, -x_qi * cos_ph_i + y_qi * sin_ph_i);
        pi_minus = vec2(-x_qi * sin_ph_i + y_qi * cos_ph_i,  x_qi * cos_ph_i + y_qi * sin_ph_i);
    }

    inline void silhouette_points_0(const vec3 wi_u, const vec3& wo_u, vec2& pi_plus, vec2& pi_minus) {
        Float x_qi = glm::sign(wi_u[0] * wo_u[1] - wo_u[0] * wi_u[1]);

        vec2 wi_u_proj = glm::normalize(vec2(wi_u.x, wi_u.y));
        Float sin_ph_i = wi_u_proj.y;
        Float cos_ph_i = wi_u_proj.x;
        pi_plus  = vec2(x_qi * sin_ph_i, -x_qi * cos_ph_i);
        pi_minus = vec2(-x_qi * sin_ph_i, x_qi * cos_ph_i);
    }

    inline bool is_left(const vec2& a, const vec2& b, const vec2& c) {
        return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]) > 0;
    }

    Float MicrograinMicrosurface::sigma_shadow_inter(const vec3& wh_u, const vec3& wi_u, const vec3& wo_u, const vec2& p, const vec2& pi, const vec2& po)
    {
        Float s_t = area_triangle(p, pi, po);
        Float s_c = area_sector(pi, po, std::sqrt(1. - wh_u[2] * wh_u[2]));
        Float s_i = area_sector_i(p, pi, wh_u[2], wi_u);
        Float s_o = area_sector_i(p, po, wh_u[2], wo_u);

        return s_t + s_i + s_o - s_c;
    }

    Float MicrograinMicrosurface::sigma_shadow_inter_0(const vec3& wi_u, const vec3& wo_u, const vec2& p, const vec2& pi, const vec2& po)
    {
        Float s_t = area_triangle(p, pi, po);
        Float s_c = area_sector(pi, po, 1.f);
        Float s_i = area_sector_i(p, pi, 0.f, wi_u);
        Float s_o = area_sector_i(p, po, 0.f, wo_u);

        return s_t + s_i + s_o - s_c;  
    }

    Float MicrograinMicrosurface::sigma_shadow_0(const vec3& wi_u)
    {
        // sigma - sigma_base
        if (wi_u.z >= 0.99999)
            return 0.;
        return pi * 0.5 * (1. / glm::clamp(wi_u.z, 0.00001f, 0.99999f) - 1.);
    }

    Float MicrograinMicrosurface::G1(const vec3& wh_u, const vec3& wi_u, const SurfaceInteraction& si)
    {
        Float tau = tau_0->eval(si);
        if(use_smith || sig_asia_2023)
            return 1. / (1. + lambda(tau,wi_u));
    
        return one_to_many(tau,sigma_shadow(wh_u, wi_u));
        //return std::exp(std::log(1. - tau_0) / pi * sigma_shadow(wh_u, wi_u));
    }

    
    Float MicrograinMicrosurface::G1_0(const vec3& wi_u, const SurfaceInteraction& si)
    {
        Float tau = tau_0->eval(si);
        return one_to_many(tau, sigma_shadow_0(wi_u));
        //return std::exp(std::log(1. - tau_0) / pi * sigma_shadow_0(wi_u));
    }


    Float MicrograinMicrosurface::G2(const vec3& wh_u, const vec3& wi_u, const vec3& wo_u, const SurfaceInteraction& si)
    {
        Float tau = tau_0->eval(si);

        if(use_smith || sig_asia_2023)
            return G1(wh_u, wi_u, si) * G1(wh_u, wo_u, si);
        
        
        Float s_i = sigma_shadow(wh_u, wi_u);
        Float s_o = sigma_shadow(wh_u, wo_u);
        Float s_shadow = s_i + s_o;
        
        if (height_and_direction && s_i > 0. && s_o > 0.) {

            // Compute orthogonal frame
            vec3 b = glm::normalize(wi_u + wo_u);
            vec3 t = glm::normalize(glm::cross(wi_u, wo_u));
            t = t * glm::sign(-t[2]);
            vec3 n = glm::normalize(glm::cross(t, b));

            // Compute silhouette points
            // for i
            vec2 pi_plus;
            vec2 pi_minus;
            silhouette_points(wh_u, wi_u, wo_u, pi_plus, pi_minus);
            // for o
            vec2 po_plus;
            vec2 po_minus;
            silhouette_points(wh_u, wo_u, wi_u, po_plus, po_minus);

            // Compute split line
            vec2 p1 = vec2(-n[2] / n[0] * wh_u[2], 0.);
            vec2 p2 = p1 + vec2(-n[1] / n[0], 1.);

            // Get relative position of all 4 silhouette points
            bool il_pi_p = is_left(p1, p2, pi_plus);
            bool il_pi_m = is_left(p1, p2, pi_minus);
            bool il_po_p = is_left(p1, p2, po_plus);
            bool il_po_m = is_left(p1, p2, po_minus);

            // We should always find : same_side_pi = same_side_po
            bool same_side_pi = il_pi_p == il_pi_m;
            bool same_side_po = il_po_p == il_po_m;
            bool same_side_pio = il_pi_p == il_po_m;
            // Find where a shadow is inside the other
            bool full = same_side_pi && same_side_po && same_side_pio;
            // Find where the shadows are not overlaped
            bool null = same_side_pi && same_side_po && (!same_side_pio);

            if (!null) {
    
                if (full){
                    s_shadow -= std::min(s_i, s_o);
                }
                
                else {
                    // Compute shadow point
                    vec2 p = shadow_intersection_point(wh_u, wi_u, wo_u, t, b);
                    s_shadow -= sigma_shadow_inter(wh_u, wi_u, wo_u, p, pi_plus, po_plus);
                }

            }            
            

        }
        return one_to_many( tau, s_shadow);
        //return std::exp(std::log(1. - tau_0) / pi * s_shadow);
    }

    Float MicrograinMicrosurface::G2_0(const vec3& wi_u, const vec3& wo_u, const SurfaceInteraction& si)
    {
        Float tau = tau_0->eval(si);

        if (use_smith || sig_asia_2023)
            return G1_0(wi_u, si) * G1_0(wo_u, si);


        Float s_i = sigma_shadow_0(wi_u);
        Float s_o = sigma_shadow_0(wo_u);
        Float s_shadow = s_i + s_o;

        if (height_and_direction && s_i > 0. && s_o > 0.) {

            // Compute orthogonal frame
            vec3 b = glm::normalize(wi_u + wo_u);
            vec3 t = glm::normalize(glm::cross(wi_u, wo_u));
            t = t * glm::sign(-t[2]);
            vec3 n = glm::normalize(glm::cross(t, b));

            // Compute silhouette points
            // for i
            vec2 pi_plus;
            vec2 pi_minus;
            silhouette_points_0(wi_u, wo_u, pi_plus, pi_minus);
            // for o
            vec2 po_plus;
            vec2 po_minus;
            silhouette_points_0(wo_u, wi_u, po_plus, po_minus);

           
            Float cos_theta_d = glm::dot(glm::normalize(vec2(wi_u)), glm::normalize(vec2(wo_u)));
            bool colinear_same_side = cos_theta_d > 0.99999f;
            bool colinear_diff_side = cos_theta_d < -0.99999f;

            if (colinear_same_side) {
                s_shadow -= std::min(s_i, s_o);
            }
            else if (!colinear_diff_side) {
                vec2 p = shadow_intersection_point_0(wi_u, wo_u, t, b);
                s_shadow -= sigma_shadow_inter_0(wi_u, wo_u, p, pi_plus, po_plus);
            }

        }
        Float G2_0_ = one_to_many( tau, s_shadow);
        //Float G2_0_ = std::exp(std::log(1. - tau_0) / pi * s_shadow);
        assert(G2_0_ == G2_0_);
        return G2_0_;
    }



}