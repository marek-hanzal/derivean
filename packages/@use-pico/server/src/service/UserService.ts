import {type IContainer}   from "../api/IContainer";
import {type IUserService} from "../api/IUserService";
import {withContainer}     from "../container/service/withContainer";
import {withUserSession}   from "../container/service/withUserSession";
import {lazyOf}            from "../utils/lazyOf";

export class UserService implements IUserService {
    static inject = [
        /**
         * Container, because user session is deferred
         */
        lazyOf(withContainer.inject),
    ];

    constructor(
        protected container: IContainer.Type,
    ) {
    }

    public optionalId(): string | undefined {
        try {
            return this.requiredId();
        } catch (e) {
            return undefined;
        }
    }

    public requiredId(): string {
        return withUserSession.use(this.container).userId;
    }
}
