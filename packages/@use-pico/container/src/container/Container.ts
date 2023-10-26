import {PumpIt}          from "pumpit";
import {type IContainer} from "../api/IContainer";

export class Container {
    constructor(
        protected container: IContainer.Instance = new PumpIt(),
    ) {
    }

    public resolve<T>(key: IContainer.Key) {
        return this.container.resolve<T>(key);
    }
}
