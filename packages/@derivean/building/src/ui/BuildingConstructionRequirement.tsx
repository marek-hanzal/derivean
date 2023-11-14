import {
    ItemInline,
    ItemUI
}                                          from "@derivean/item";
import {t}                                 from "@use-pico/i18n";
import {
    NativeBreadcrumbs,
    Text
}                                          from "@use-pico/ui";
import {type FC}                           from "react";
import {BuildingConstructionRequirementUI} from "./BuildingConstructionRequirementUI";

export namespace BuildingConstructionRequirement {
    export interface Props {
        buildingId: string;
    }
}

export const BuildingConstructionRequirement: FC<BuildingConstructionRequirement.Props> = (
    {
        buildingId,
    }
) => {
    return <BuildingConstructionRequirementUI.Collection
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
                {entities.length > 0 && entities.map(entity => <ItemUI.Fetch
                    key={entity.id}
                    override={entity.itemId}
                    WithSuccess={({entity}) => <ItemInline entity={entity}/>}
                />)}
                {!entities.length && <Text c={"dimmed"}>({t()`Building without requirements`})</Text>}
            </NativeBreadcrumbs>;
        }}
    />;
};
