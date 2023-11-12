import {
    ResourceInline,
    ResourceUI
}                              from "@derivean/resource";
import {NativeBreadcrumbs}     from "@use-pico/ui";
import {type FC}               from "react";
import {BuildingRequirementUI} from "./BuildingRequirementUI";

export namespace BuildingRequirement {
    export interface Props {
        buildingId: string;
    }
}

export const BuildingRequirement: FC<BuildingRequirement.Props> = (
    {
        buildingId,
    }
) => {
    return <BuildingRequirementUI.Collection
        query={{
            where: {
                buildingId,
            }
        }}
        WithSuccess={({entities}) => {
            return <NativeBreadcrumbs
                separator={"&"}
                separatorMargin={4}
            >
                {entities.map(entity => <ResourceUI.Fetch
                    key={entity.id}
                    override={entity.resourceId}
                    WithSuccess={({entity}) => <ResourceInline entity={entity}/>}
                />)}
            </NativeBreadcrumbs>;
        }}
    />;
};
