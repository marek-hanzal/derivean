import {withBuildingRepository} from "@derivean/building";
import {container}              from "@derivean/server";
import {BuildingIcon}           from "@derivean/ui";
import {
    Breadcrumbs,
    HomeIcon,
    ListIcon,
    Page
}                               from "@use-pico/ui";

export namespace Index {
    export interface Props {
        params: {
            id: string;
        };
    }
}

export default async function Index({params: {id}}: Index.Props) {
    const building = await withBuildingRepository.use(container).withQuery.fetchOrThrow({
        where: {
            id,
        }
    });

    return <Page
        icon={<BuildingIcon/>}
        title={"manager.building.detail"}
        withTranslation={{
            values: building,
        }}
        postfix={<Breadcrumbs
            items={{
                "/manager":               {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
                "/manager/building/list": {
                    type:  "link",
                    href:  "/manager/building/list",
                    label: "manager.building.list.title",
                    icon:  <ListIcon/>,
                },
            }}
        />}
    >
        {building.name}
    </Page>;
}