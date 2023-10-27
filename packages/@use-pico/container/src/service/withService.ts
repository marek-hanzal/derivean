import {type FactoryValue} from "pumpit";
import {type IContainer}   from "../api/IContainer";

export namespace withService {
    export interface Service<TService> {
        key: symbol;

        resolve(container: IContainer.Type): TService;

        bind<T extends new (...args: any) => TService>(container: IContainer.Type, value: T, options?: IContainer.Options.Class<T>): void;

        factory<T extends FactoryValue>(container: IContainer.Type, factory: T, options?: IContainer.Options.Factory<T>): void;

        use(container: IContainer.Type, value: TService): void;
    }
}

export const withService = <TService>(key: string): withService.Service<TService> => {
    return {
        key: Symbol.for(key),
        resolve(container) {
            return container.resolve<TService>(this.key);
        },
        bind(container, value, options) {
            container.useClass(this.key, value, options);
        },
        factory(container, factory, options) {
            container.useFactory(this.key, factory, options);
        },
        use(container, value) {
            container.useValue(this.key, value);
        },
    };
};
