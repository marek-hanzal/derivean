import {type IToken}            from "@use-pico/auth";
import {type IUserTokenService} from "../api/IUserTokenService";

export class UserTokenService implements IUserTokenService {
    public async token<T extends IToken>(props: T): Promise<T> {
        return {
            ...props,
            tokens: this.defaults(),
        };
    }

    public defaults(): string[] {
        return ["user"];
    }
}
