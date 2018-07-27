''' Predict the interior temperature of a Ford Focus parked in direct summer
sunlight. All unit are m-kg-s-K'''

from numpy import cos
import numpy as np
from matplotlib import pyplot as plt

def Ra(T_s, T_amb, L_c, nu, Pr, theta=0):
    ''' Calculate Rayleigh number.
        T_s = surface temperature,
        T_amb = ambient temperature
        L_c = characteristic length
        nu = viscosity of air
        Pr = Pandtl number of air
        theta = angle of surface from vertical
    '''
    g = 9.81
    theta *= np.pi/180.
    T_f = (T_s + T_amb)/2.
    beta = 1/T_f
    return g*cos(theta)*beta*(T_s - T_amb) * L_c**3 * Pr/ nu**2

def Nu_vert(Ra, Pr):
    ''' Calculates Nusselt number for a vertical surface under natural convection
        Ra = rayleigh number
        Pr = Prandtl number
        '''
    return ( .825 + .387*Ra**(1/6)/(1 + (.492/Pr)**(9/16))**(8/27) )**2

def Nu_hor(Ra, Pr):
    ''' Calculates Nusselt number for a horizontal surface under natural convection
        Ra = rayleigh number
        Pr = Prandtl number
        '''
    if Ra < 10**7:
        return .54*Ra**.25
    elif Ra < 10**11:
        return .15*Ra**.333
    else:
        return Exception("Raleigh number outside of range")

def q_net(alpha, T_s, e, G, T_surround):
    ''' Calculate the net radiation into a surface. This is the sunlight minus
        blackbody radiation of the surface to its surroundings.
        alpha = absorptivity of surface
        T_s = surface temperature
        e = emissivity
        G = solar radiation onto surface
        T_surround = temperature of objects opposite surface
        (sky, trees, ground, etc)
        '''
    # Stefan-Boltzman
    sigma = 5.67e-8

    # Effective sky temperature as measured with thermopile for reference
    T_sky = 280. #K

    return alpha*G + e*sigma*(T_surround**4 - T_s**4)


#ambient temp
T_amb = (83.-32)*(5./9)+273.15

#viscosity of air
nu = 1.4e-5

#prandtl number of air
Pr = .73

#conductivity of air
k = .024



# Windshield
T_sur = 273.+45.   #surface temp as measured with thermocouple
alpha = .87        #abosrptivity of glass
e = .87            #emissivity of glass
W = 4*12*.0254     #width
H = 2.5*12*.0254   #height
A = W*H            #surface area
L_c = A/(2*(W+H))  #characteristice length
Ra_wind = Ra(T_sur, T_amb, L_c, nu, Pr, theta=70)    #rayleigh number
h_wind = Nu_hor(Ra_wind, Pr)*k/L_c                   #heat transfer coefficient
U_wind = h_wind*A                                    #overall heat transfer coefficient
Q_rad_wind = q_net(alpha, T_sur, e, 949., T_amb)*A   #net radiation

''' net radiation numbers are dependent on the time of year and direction
the car is parked. The numbers used in Q_rad variables are from ASHRAE tables
for July at 40 degrees latitude at noon with the car's windshield facing south

For the test the car was parked in direct sunlight, but trees were occluding the
top of the car from the sky, so the ambient temperature is used as the
surrounding temperature instead of the effective sky temperature.
'''


# side windows
T_sur = 40+273.
W = 5*12*.0254
H = 1.5*12*.0254
A = W*H
A_conv = (5*12*.0254)**2
L_c = 5*12*.0254
Ra_window = Ra(T_sur, T_amb, L_c, nu, Pr)
h_window = Nu_vert(Ra_window, Pr)*k/L_c
U_window = h_window*A_conv
Q_rad_window_driver = q_net(alpha, T_sur, e, 395., T_amb)*A
Q_rad_window_passenger = q_net(alpha, T_sur, e, 149., T_amb)*A

# roof
alpha = .5
e = .93
T_sur = 45+273.
W = 5*12*.0254
D = 5*12*.0254
A = W*D
L_c = A/(2*(W+D))
Ra_roof = Ra(T_sur, T_amb, L_c, nu, Pr)
h_roof = Nu_hor(Ra_roof, Pr)
U_roof = h_roof*A
Q_rad_roof = q_net(alpha, T_sur, e, 939., T_amb)*A

# total up the convection. double the windshield for front and rear.
# double the windows for driver and passenger side
U_total = 2*U_wind + 2*U_window + U_roof
Q_dot = 2*Q_rad_wind + Q_rad_window_driver + Q_rad_window_passenger + Q_rad_roof
T_inside = Q_dot/U_total + T_amb


print "T_inside = ", T_inside
T_inside_F = (T_inside - 273.15)*(9/5.) + 32.
print "T_inside = ", T_inside_F

'''
This model was tested by placing a temperature sensor inside the car. The first
day ambient temperatures reached 83 F. The model predicted an interior temperature
of 109, and 106 was measured. On the second day temperatures were 95 F, and the
interior temp was measured as 131 F and predicted to be 128 F.

'''
