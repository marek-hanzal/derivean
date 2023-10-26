import {type IContainer} from "../api/IContainer";

export namespace withService {
    export interface Service<TService> {
        key: symbol;

        resolve(container: IContainer.Type): TService;

        bind<T extends new (...args: any) => TService>(container: IContainer.Type, value: T): void;

        use(container: IContainer.Type, value: TService): void;
    }
}

export const withService = <TService>(key: string): withService.Service<TService> => {
    return {
        key: Symbol.for(key),
        resolve(container) {
            return container.resolve<TService>(this.key);
        },
        bind(container, value) {
            container.useClass(this.key, value);
        },
        use(container, value) {
            container.useValue(this.key, value);
        },
    };
};
