#ID: ctl-mixed
#TYPE: controller
#NAME: Dummy mixed type controller

import env

class Controller:
    def __init__(self, team_id):
        self.team_id = team_id

    def initialize(self):
        count = env.control.get_robot_count(self.team_id)
        for i in range(int(count/2)):
            env.control.set_robot_script(self.team_id, i, 'random')
        for i in range(int(count/2), int(count)):
            env.control.set_robot_script(self.team_id, i, 'swing')

    def update(self):
        pass
