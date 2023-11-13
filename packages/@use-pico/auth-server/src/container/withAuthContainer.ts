import {type IContainer}         from "@use-pico/container";
import {RegistrationService}     from "../service/RegistrationService";
import {UserService}             from "../service/UserService";
import {UserTokenService}        from "../service/UserTokenService";
import {withRegistrationService} from "./withRegistrationService";
import {withUserService}         from "./withUserService";
import {withUserTokenService}    from "./withUserTokenService";

export const withAuthContainer: IContainer.Register = container => {
    withRegistrationService.bind(container, RegistrationService);
    withUserTokenService.bind(container, UserTokenService);
    withUserService.bind(container, UserService);
};
