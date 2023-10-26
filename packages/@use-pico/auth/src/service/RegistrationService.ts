import {type IRegistrationService} from "../api/IRegistrationService";
import {type IToken}               from "../api/IToken";

export class RegistrationService implements IRegistrationService {
    public handle<T extends IToken>(props: IRegistrationService.HandleProps<T>): Promise<void> {
        return Promise.resolve(undefined);
    }
}
