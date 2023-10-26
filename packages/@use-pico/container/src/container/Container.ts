import {
    type AvailableScopes,
    type ClassOptions,
    type ClassValue,
    PumpIt
}                        from "pumpit";
import {type IContainer} from "../api/IContainer";

export class Container {
    constructor(
        protected container: IContainer.Instance = new PumpIt(),
    ) {
    }

    public resolve<T>(key: IContainer.Key) {
        return this.container.resolve<T>(key);
    }

    public useClass<TClass extends ClassValue>(key: IContainer.Key, use: TClass, options?: Omit<Partial<ClassOptions<TClass, AvailableScopes>>, "type">) {
        this.container.bindClass(key, use, options);
        return this;
    }

    public useValue<T>(key: IContainer.Key, value: T) {
        this.container.bindValue(key, value);
        return this;
    }
}
