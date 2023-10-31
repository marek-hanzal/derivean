import {type IContainer}        from "@use-pico/container";
import {withResourceRepository} from "../repository/withResourceRepository";

export const withResourceContainer: IContainer.Register = container => {
    withResourceRepository(container);
};
