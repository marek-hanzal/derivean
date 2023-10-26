import {type IContainer}           from "@use-pico/container";
import {type IRegistrationService} from "../api/IRegistrationService";
import {withRegistrationService}   from "../api/withRegistrationService";

export const useRegistrationService = (container: IContainer.Type) => container.resolve<IRegistrationService>(withRegistrationService);
